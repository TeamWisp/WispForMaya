#include "material_parser.hpp"

#include "plugin/callback_manager.hpp"
#include "plugin/renderer/material_manager.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Maya API
#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnTransform.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

// C++ standard
#include <string>
#include <vector>

#include <maya/MGlobal.h>
#include <sstream>

wmr::MaterialParser::MaterialParser() :
	m_renderer(dynamic_cast<const ViewportRendererOverride*>(
	MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME)
	)->GetRenderer())
{

}

namespace wmr
{
	void DirtyNodeCallback(MObject &node, MPlug &plug, void *clientData)
	{
		if (node.hasFn(MFn::kLambert))
		{
			wmr::MaterialParser *material_parser = reinterpret_cast<wmr::MaterialParser*>(clientData);

			MStatus status;
			MFnLambertShader shader(plug.node(), &status);
			if (status != MS::kSuccess)
			{
				MGlobal::displayInfo(status.errorString());
				return;
			}

			std::optional<MObject> mesh_object = material_parser->GetMeshObjectFromMaterial(node);
			if (!mesh_object.has_value())
			{
				MGlobal::displayInfo("A connect to a material could not be found! (wmr::DirtyNodeCallback)");
				return;
			}

			wr::Material *material = material_parser->GetRenderer().GetMaterialManager().GetMaterial(mesh_object.value());

		}
		else if (node.hasFn(MFn::kPhong))
		{
			MGlobal::displayInfo("Phong!");
		}
	}
} /* namespace wmr */



// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::Parse(const MFnMesh& mesh)
{
	auto material_manager = m_renderer.GetMaterialManager();
	auto texture_manager = m_renderer.GetTextureManager();

	// Number of instances of this mesh
	std::uint32_t instance_count = mesh.parentCount();

	for (auto instance_index = 0; instance_index < instance_count; ++instance_index)
	{
		// Attach a function set to the instance
		MFnDependencyNode mesh_fn(mesh.parent(instance_index));

		// References to the shaders used on the meshes
		MObjectArray shaders;

		// Indices to the materials in the object array
		MIntArray material_indices;

		// Get all attached shaders for this instance
		mesh.getConnectedShaders(instance_index, shaders, material_indices);

		switch (shaders.length())
		{
			// No shaders applied to this mesh instance
			case 0:
				{
				}
				break;

				// All faces use the same material
			case 1:
				{
					// Output hack
					std::ostringstream os;

					auto surface_shader = GetSurfaceShader(shaders[0]);

					// Invalid surface shader
					if (!surface_shader.has_value())
						return;

					// Find all plugs that are connected to this shader
					MPlugArray connected_plugs;
					surface_shader.value().connectedTo(connected_plugs, true, false);

					// Could not find a valid connected plug
					if (connected_plugs.length() != 1)
						return;

					auto shader_type = GetShaderType(connected_plugs[0].node());

					// Shader type not supported by this plug-in
					if (shader_type == detail::SurfaceShaderType::UNSUPPORTED)
						return;

					// Found a Lambert shader
					if (shader_type == detail::SurfaceShaderType::LAMBERT)
					{
						os << "Found a Lambert shader!" << std::endl;

						MObject connected_plug = connected_plugs[0].node();
						auto color_plug = GetPlugByName(connected_plug, "color");

						// Retrieve the texture associated with this plug
						auto albedo_texture_path = GetPlugTexture(color_plug);

						MObject object = mesh.object();
						wr::MaterialHandle material_handle = material_manager.DoesExist(object);

						// Invalid material handle, create a new one
						if (!material_handle.m_pool)
						{
							material_handle = material_manager.CreateMaterial(object);
						}

						mesh_material_relations.push_back(std::make_pair(connected_plug, object));

						// Get a Wisp material for this handle
						auto material = material_manager.GetMaterial(material_handle);

						// Add callback that filters on material changes
						MStatus status;
						MCallbackId attributeId = MNodeMessage::addNodeDirtyCallback(
							connected_plug,
							DirtyNodeCallback,
							this,
							&status
						);

						CallbackManager::GetInstance().RegisterCallback(attributeId);

						// If there is no color available, use the RGBA values
						if (albedo_texture_path == "")
						{
							MColor albedo_color;

							MFnDependencyNode dep_node_fn(connected_plug);
							dep_node_fn.findPlug("colorR").getValue(albedo_color.r);
							dep_node_fn.findPlug("colorG").getValue(albedo_color.g);
							dep_node_fn.findPlug("colorB").getValue(albedo_color.b);

							material->SetConstantAlbedo({ albedo_color.r, albedo_color.g, albedo_color.b });
							material->SetUseConstantAlbedo(true);
						}
						else
						{
							// The name of the object is needed when constructing an unique name for the texture
							std::string mesh_name = mesh_fn.name().asChar();

							// Unique names for the textures
							std::string albedo_name = mesh_name + "_albedo";

							// Request new Wisp textures
							auto albedo_texture = texture_manager.CreateTexture(albedo_name.c_str(), albedo_texture_path.asChar());

							// Use this texture as the material albedo texture
							material->SetAlbedo(*albedo_texture);
							material->SetUseConstantAlbedo(false);

							// Print the texture location
							os << albedo_texture_path.asChar() << std::endl;
						}
					}

					// Log the string stream to the Maya script output window
					MGlobal::displayInfo(os.str().c_str());
				}
				break;

				// Two or more materials are used
			default:
				{
					// Get the number of materials by doing:
					// auto num_of_mats = shaders.length()

					// Holds sorted face indices based on their applied material
					std::vector<std::vector<std::uint32_t>> faces_by_material_index;

					// Make sure the container has the same size as the number of shaders
					faces_by_material_index.resize(shaders.length());

					for (auto material_index = 0; material_index < material_indices.length(); ++material_index)
					{
						faces_by_material_index[material_indices[material_index]].push_back(material_index);
					}

					for (auto shader_index = 0; shader_index < shaders.length(); ++shader_index)
					{
						// Get all faces used by this material index
						for (unsigned int & itr : faces_by_material_index[shader_index])
						{
							// Use faces by material ID here
						}
					}
				}
				break;
		}
	}
}

const wmr::detail::SurfaceShaderType wmr::MaterialParser::GetShaderType(const MObject& node)
{
	detail::SurfaceShaderType shader_type = detail::SurfaceShaderType::UNSUPPORTED;

	switch (node.apiType())
	{
		case MFn::kLambert:
			shader_type = detail::SurfaceShaderType::LAMBERT;
			break;

		case MFn::kPhong:
			shader_type = detail::SurfaceShaderType::PHONG;
			break;

		default:
			break;
	}

	return shader_type;
}

const MString wmr::MaterialParser::GetPlugTexture(MPlug& plug)
{
	MItDependencyGraph dependency_graph_iterator(
		plug,
		MFn::kFileTexture,
		MItDependencyGraph::kUpstream,
		MItDependencyGraph::kBreadthFirst,
		MItDependencyGraph::kNodeLevel);

	dependency_graph_iterator.disablePruningOnFilter();

	auto texture_node = dependency_graph_iterator.currentItem();
	auto file_name_plug = MFnDependencyNode(texture_node).findPlug("fileTextureName", true);

	MString texture_path;
	file_name_plug.getValue(texture_path);

	return texture_path;
}

const MPlug wmr::MaterialParser::GetPlugByName(const MObject& node, MString name)
{
	return MFnDependencyNode(node).findPlug(name);
}

const std::optional<MPlug> wmr::MaterialParser::GetSurfaceShader(const MObject& node)
{
	MPlug shader_plug = MFnDependencyNode(node).findPlug("surfaceShader", true);

	if (!shader_plug.isNull())
		return shader_plug;
	else
		return std::nullopt;
}

const std::optional<MObject> wmr::MaterialParser::GetMeshObjectFromMaterial(MObject & object)
{
	auto it = std::find_if(mesh_material_relations.begin(), mesh_material_relations.end(), [&object] (std::pair<MObject, MObject> pair)
	{
		return (pair.first == object);
	});

	auto end_it = --mesh_material_relations.end();

	if (it != end_it)
	{
		// Material does not exist!
		return std::nullopt;
	}

	return it->second;
}

const wmr::Renderer & wmr::MaterialParser::GetRenderer()
{
	return m_renderer;
}
