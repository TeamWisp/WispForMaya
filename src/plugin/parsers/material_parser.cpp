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
		//// Get material parser from client data
		//wmr::MaterialParser *material_parser = reinterpret_cast<wmr::MaterialParser*>(clientData);
		//
		//// Get Maya mesh object that is using the Maya material
		//std::optional<MObject> mesh_object = material_parser->GetMeshObjectFromMaterial(node);
		//if (!mesh_object.has_value())
		//{
		//	MGlobal::displayError("A connect to a material could not be found! (wmr::DirtyNodeCallback)");
		//	return;
		//}

		//// Get Wisp material that is used by the found Maya mesh object
		//wr::Material *material = material_parser->GetRenderer().GetMaterialManager().GetMaterial(mesh_object.value());

		//// Get plug name that has changed value
		//MString changedPlugName = plug.partialName(false, false, false, false, false, true);

		//MFnDependencyNode fn_material(node);
		//if (material != nullptr)
		//{
		//	if (node.hasFn(MFn::kLambert))
		//	{
		//		material_parser->HandleLambertChange(fn_material, plug, changedPlugName, *material);
		//	}
		//	else if (node.hasFn(MFn::kPhong))
		//	{
		//		material_parser->HandlePhongChange(fn_material, plug, changedPlugName, *material);
		//	}
		//}
	}
} /* namespace wmr */



// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::OnMeshAdded(const MFnMesh& mesh)
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
					auto shading_engine = shaders[0];

					// Get shader plug from shading engine
					auto opt_surface_shader_plug = GetSurfaceShader(shading_engine);
					if (!opt_surface_shader_plug.has_value())
						return;
					auto surface_shader_plug = opt_surface_shader_plug.value();

					// Get the actual surface shader plug
					auto opt_actual_surface_shader = GetActualSurfaceShaderPlug(surface_shader_plug);
					if (!opt_surface_shader_plug.has_value())
						return;
					auto actual_surface_shader = opt_actual_surface_shader.value();

					// Check if shader type not supported by this plug-in
					auto shader_type = GetShaderType(actual_surface_shader.node());
					if (shader_type == detail::SurfaceShaderType::UNSUPPORTED)
						return;

					// Wisp material
					SurfaceShaderShadingEngineRelation *shader_relation = material_manager.DoesSurfaceShaderExist(surface_shader_plug);
					MObject mesh_object = mesh.object();
					wr::MaterialHandle material_handle = material_manager.CreateMaterial(mesh_object, shading_engine, surface_shader_plug);

					// Get a Wisp material for this handle
					auto material = material_manager.GetWispMaterial(material_handle);

					// Arnold PBR standard surface shader
					if (shader_type == detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER)
					{
						auto data = ParseArnoldStandardSurfaceShaderData(actual_surface_shader);

						// Add callback that filters on material changes
						SubscribeSurfaceShader(actual_surface_shader.node(), shading_engine);

						ConfigureWispMaterial(data, material, mesh_fn.name(), texture_manager);
					}

					// Found a Lambert shader
					if (shader_type == detail::SurfaceShaderType::LAMBERT)
					{
						// Add callback that filters on material changes
						SubscribeSurfaceShader(actual_surface_shader.node(), shading_engine);

						// Retrieve the texture associated with this plug
						auto color_plug = GetPlugByName(actual_surface_shader.node(), MayaMaterialProps::plug_color);
						auto albedo_texture_path = GetPlugTexture(color_plug);
						
						// If there is no color available, use the RGBA values
						if (!albedo_texture_path.has_value())
						{

							MFnDependencyNode dep_node_fn(actual_surface_shader.node());
							MString color_str(MayaMaterialProps::plug_color);
							MColor albedo_color = GetColor(dep_node_fn, color_str);
							
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
							auto albedo_texture = texture_manager.CreateTexture(albedo_name.c_str(), albedo_texture_path.value().asChar());

							// Use this texture as the material albedo texture
							material->SetAlbedo(*albedo_texture);
							material->SetUseConstantAlbedo(false);
						}
					}
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

void wmr::MaterialParser::ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().ConnectShaderToShadingEngine(surface_shader, shading_engine);
}

void wmr::MaterialParser::DisconnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().DisconnectShaderFromShadingEngine(surface_shader, shading_engine);
}

void wmr::MaterialParser::ConnectMeshToShadingEngine(MFnMesh & fnmesh, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().ConnectMeshToShadingEngine(fnmesh, shading_engine);
}

void wmr::MaterialParser::DisconnectMeshFromShadingEngine(MFnMesh & fnmesh, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().DisconnectMeshFromShadingEngine(fnmesh, shading_engine);
}

const wmr::detail::SurfaceShaderType wmr::MaterialParser::GetShaderType(const MObject& node)
{
	auto node_fn = MFnDependencyNode(node);
	auto shader_type_name = node_fn.typeName();

	if (shader_type_name == MayaMaterialProps::maya_phong_shader_name)
	{
		return detail::SurfaceShaderType::PHONG;
	}
	else if (shader_type_name == MayaMaterialProps::maya_lambert_shader_name)
	{
		return detail::SurfaceShaderType::LAMBERT;
	}
	else if (shader_type_name == MayaMaterialProps::arnold_standard_shader_name)
	{
		return detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER;
	}
	else
	{
		return detail::SurfaceShaderType::UNSUPPORTED;
	}
}

const std::optional<MString> wmr::MaterialParser::GetPlugTexture(MPlug& plug)
{
	MItDependencyGraph dependency_graph_iterator(
		plug,
		MFn::kFileTexture,
		MItDependencyGraph::kUpstream,
		MItDependencyGraph::kBreadthFirst,
		MItDependencyGraph::kNodeLevel);

	dependency_graph_iterator.disablePruningOnFilter();

	auto texture_node = dependency_graph_iterator.currentItem();
	// Check if texture was found
	if (texture_node.apiType() == MFn::Type::kInvalid)
	{
		return std::nullopt;
	}
	auto file_name_plug = MFnDependencyNode(texture_node).findPlug(MayaMaterialProps::plug_file_texture_name, true);
	auto type = file_name_plug.node().apiType();

	MString texture_path;
	file_name_plug.getValue(texture_path);
	// No texture has been found
	if (texture_path.length() <= 0)
	{
		return std::nullopt;
	}
	return texture_path;
}

const MPlug wmr::MaterialParser::GetPlugByName(const MObject& node, MString name)
{
	return MFnDependencyNode(node).findPlug(name);
}

const std::optional<MPlug> wmr::MaterialParser::GetSurfaceShader(const MObject& node)
{
	MPlug shader_plug = MFnDependencyNode(node).findPlug(MayaMaterialProps::surface_shader, true);

	if (!shader_plug.isNull())
		return shader_plug;
	else
		return std::nullopt;
}

const std::optional<MPlug> wmr::MaterialParser::GetActualSurfaceShaderPlug(const MPlug & surface_shader_plug)
{
	MPlugArray connected_plugs;
	surface_shader_plug.connectedTo(connected_plugs, true, false);

	// Could not find a valid connected plug
	auto num = connected_plugs.length();
	if (num <= 0)
		return std::nullopt;

	return connected_plugs[0];
}

const void wmr::MaterialParser::SubscribeSurfaceShader(MObject actual_surface_shader, MObject shading_engine)
{
	MaterialParser::ShaderDirtyData * data = new MaterialParser::ShaderDirtyData();
	data->material_parser = this;
	data->shading_engine = shading_engine;

	MStatus status;
	MCallbackId addedId = MNodeMessage::addNodeDirtyCallback(
		actual_surface_shader,
		DirtyNodeCallback,
		this,
		&status
	);

	if (status != MS::kSuccess)
	{
		delete data;
	}
	else
	{
		CallbackManager::GetInstance().RegisterCallback(addedId);
		data->callback_id = addedId;

		shader_dirty_datas.push_back(data);
	}
}

void wmr::MaterialParser::ConfigureWispMaterial(const wmr::detail::ArnoldStandardSurfaceShaderData& data, wr::Material* material, MString mesh_name, TextureManager& texture_manager) const
{
	material->SetUseConstantAlbedo(data.using_diffuse_color_value);
	material->SetUseConstantMetallic(data.using_metalness_value);
	material->SetUseConstantRoughness(data.using_diffuse_roughness_value);

	if (data.using_diffuse_color_value)
	{
		material->SetConstantAlbedo({ data.diffuse_color[0], data.diffuse_color[1], data.diffuse_color[2] });
	}
	else
	{
		// Unique names for the textures
		std::string albedo_name = std::string(mesh_name.asChar()) + "_albedo";

		// Request new Wisp textures
		auto albedo_texture = texture_manager.CreateTexture(albedo_name.c_str(), data.diffuse_color_texture_path);

		// Use this texture as the material albedo texture
		material->SetAlbedo(*albedo_texture);
	}

	if (data.using_diffuse_roughness_value)
	{
		material->SetConstantRoughness(data.diffuse_roughness);
	}
	else
	{
		// Unique names for the textures
		std::string roughness_name = std::string(mesh_name.asChar()) + "_roughness";

		// Request new Wisp textures
		auto roughness_texture = texture_manager.CreateTexture(roughness_name.c_str(), data.diffuse_roughness_texture_path);

		// Use this texture as the material albedo texture
		material->SetRoughness(*roughness_texture);
	}

	if (data.using_metalness_value)
	{
		material->SetConstantMetallic({ data.metalness, 0.0f, 0.0f });
	}
	else
	{
		// Unique names for the textures
		std::string metalness_name = std::string(mesh_name.asChar()) + "_metalness";

		// Request new Wisp textures
		auto metalness_texture = texture_manager.CreateTexture(metalness_name.c_str(), data.metalness_texture_path);

		// Use this texture as the material albedo texture
		material->SetMetallic(*metalness_texture);
	}

	// #TODO: TAHAR --> normal / bump map!
}

wmr::detail::ArnoldStandardSurfaceShaderData wmr::MaterialParser::ParseArnoldStandardSurfaceShaderData(const MObject& plug)
{
	detail::ArnoldStandardSurfaceShaderData data = {};

	// Need this to get access to functions that allow us to retrieve the data from the plug
	MFnDependencyNode dep_node_fn(plug);

	// Get all PBR variables
	auto diffuse_color_plug			= dep_node_fn.findPlug(plug, detail::ArnoldStandardSurfaceShaderData::diffuse_color_plug_name);
	auto diffuse_roughness_plug		= dep_node_fn.findPlug(plug, detail::ArnoldStandardSurfaceShaderData::diffuse_roughness_plug_name);
	auto metalness_plug				= dep_node_fn.findPlug(plug, detail::ArnoldStandardSurfaceShaderData::metalness_plug_name);
	auto specular_color_plug		= dep_node_fn.findPlug(plug, detail::ArnoldStandardSurfaceShaderData::specular_color_plug_name);
	auto specular_roughness_plug	= dep_node_fn.findPlug(plug, detail::ArnoldStandardSurfaceShaderData::specular_roughness_plug_name);

	// Attempt to retrieve a texture for each PBR variable
	auto diffuse_color_texture_path			= GetPlugTexture(diffuse_color_plug);
	auto diffuse_roughness_texture_path		= GetPlugTexture(diffuse_roughness_plug);
	auto metalness_texture_path				= GetPlugTexture(metalness_plug);
	auto specular_color_texture_path		= GetPlugTexture(specular_color_plug);
	auto specular_roughness_texture_path	= GetPlugTexture(specular_roughness_plug);

	// Diffuse color
	if (diffuse_color_texture_path.has_value())
	{
		data.using_diffuse_color_value = false;
		data.diffuse_color_texture_path = diffuse_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = detail::ArnoldStandardSurfaceShaderData::diffuse_color_plug_name;

		auto color = GetColor(dep_node_fn, plug_name);

		data.diffuse_color[0] = color.r;
		data.diffuse_color[1] = color.g;
		data.diffuse_color[2] = color.b;
	}

	// Diffuse roughness
	if (diffuse_roughness_texture_path.has_value())
	{
		data.using_diffuse_roughness_value = false;
		data.diffuse_roughness_texture_path = diffuse_roughness_texture_path.value().asChar();
	}
	else
	{
		dep_node_fn.findPlug(detail::ArnoldStandardSurfaceShaderData::diffuse_roughness_plug_name).getValue(data.diffuse_roughness);
	}

	// Metalness
	if (metalness_texture_path.has_value())
	{
		data.using_metalness_value = false;
		data.metalness_texture_path = metalness_texture_path.value().asChar();
	}
	else
	{
		dep_node_fn.findPlug(detail::ArnoldStandardSurfaceShaderData::metalness_plug_name).getValue(data.metalness);
	}

	// Specular color
	if (specular_color_texture_path.has_value())
	{
		data.using_specular_color_value = false;
		data.specular_color_texture_path = specular_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = detail::ArnoldStandardSurfaceShaderData::specular_color_plug_name;

		auto color = GetColor(dep_node_fn, plug_name);

		data.specular_color[0] = color.r;
		data.specular_color[1] = color.g;
		data.specular_color[2] = color.b;
	}

	// Specular roughness
	if (specular_roughness_texture_path.has_value())
	{
		data.using_specular_roughness_value = false;
		data.specular_roughness_texture_path = diffuse_color_texture_path.value().asChar();
	}
	else
	{
		dep_node_fn.findPlug(detail::ArnoldStandardSurfaceShaderData::specular_roughness_plug_name).getValue(data.specular_roughness);
	}

	return data;
}

MColor wmr::MaterialParser::GetColor(MFnDependencyNode & fn, MString & plug_name)
{
	MColor color;

	// get a plug to the attribute
	fn.findPlug(plug_name + MayaMaterialProps::plug_color_r).getValue(color.r);
	fn.findPlug(plug_name + MayaMaterialProps::plug_color_g).getValue(color.g);
	fn.findPlug(plug_name + MayaMaterialProps::plug_color_b).getValue(color.b);

	return color;
}

void wmr::MaterialParser::HandleLambertChange(MFnDependencyNode & fn, MPlug & plug, MString & plug_name, wr::Material & material)
{
	if (strcmp(plug_name.asChar(), MayaMaterialProps::plug_color) == 0)
	{
		MColor color = GetColor(fn, plug_name);
		material.SetConstantAlbedo({color.r, color.g, color.b});
		material.SetUseConstantAlbedo(true);
	}
}

void wmr::MaterialParser::HandlePhongChange(MFnDependencyNode & fn, MPlug & plug, MString & plug_name, wr::Material & material)
{
	if (strcmp(plug_name.asChar(), MayaMaterialProps::plug_color) == 0)
	{
		MColor color = GetColor(fn, plug_name);
		material.SetConstantAlbedo({color.r, color.g, color.b});
		material.SetUseConstantAlbedo(true);
	}
	else if (strcmp(plug_name.asChar(), MayaMaterialProps::plug_reflectivity))
	{
		float reflectivity = 0.0f;
		fn.findPlug(MayaMaterialProps::plug_reflectivity).getValue(reflectivity);
		material.SetConstantMetallic({reflectivity, reflectivity, reflectivity});
		material.SetUseConstantMetallic(true);
	}
}

const wmr::Renderer & wmr::MaterialParser::GetRenderer()
{
	return m_renderer;
}
