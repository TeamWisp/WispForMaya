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

void MaterialCallback(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData)
{
	MGlobal::displayInfo("Hey! Im a material callback!");
}

// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::Parse(const MFnMesh& mesh)
{
	auto material_manager = m_renderer.GetMaterialManager();
	auto texture_manager = m_renderer.GetTextureManager();

	// Number of instances of this mesh
	std::uint32_t instance_count = mesh.parentCount();

	for (auto instance_index = 0; instance_index < instance_count; ++instance_index)
	{
		auto instance = mesh.parent(instance_index);

		// Attach a function set to the instance
		MFnDependencyNode mesh_fn(instance);

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

						auto color_plug = GetPlugByName(connected_plugs[0].node(), "color");

						// Retrieve the texture associated with this plug
						auto albedo_texture_path = GetPlugTexture(color_plug);

						// The name of the object is needed when constructing an unique name for the texture
						std::string mesh_name = mesh_fn.name().asChar();

						// Unique names for the textures
						std::string albedo_name = mesh_name + "_albedo";

						// Request new Wisp textures
						auto albedo_texture = texture_manager.CreateTexture(albedo_name.c_str(), albedo_texture_path.asChar());

						// Print the texture location
						os << albedo_texture_path.asChar() << std::endl;

						wr::MaterialHandle material_handle = material_manager.DoesExist(instance);
						if (material_handle.m_pool == nullptr)
						{
							MObject transform = GetTransformFromFnMesh(mesh);
							material_handle = material_manager.CreateMaterial(transform);

							auto material = material_manager.GetMaterial(material_handle);
							material->SetAlbedo(*albedo_texture);
						}

						// Add callback that filters on material changes
						MStatus status;
						MCallbackId attributeId = MNodeMessage::addAttributeChangedCallback(
							instance,
							MaterialCallback,
							this,
							&status
						);
						CallbackManager::GetInstance().RegisterCallback(attributeId);
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
