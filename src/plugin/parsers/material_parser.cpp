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
#include <maya/MSelectionList.h>

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
		// Get material parser, material manager, and material handle from client data
		wmr::MaterialParser::ShaderDirtyData *shader_dirty_shader = reinterpret_cast<wmr::MaterialParser::ShaderDirtyData*>(clientData);
		wmr::MaterialParser * material_parser = shader_dirty_shader->material_parser;
		wmr::MaterialManager & material_manager = material_parser->GetRenderer().GetMaterialManager();

		// Check if surface shader exists. Quit the callback if it doesn't exists
		wmr::SurfaceShaderShadingEngineRelation * relation = material_manager.DoesSurfaceShaderExist(node);
		if (relation == nullptr)
		{
			return;
		}
		wr::MaterialHandle material_handle = relation->material_handle;

		// Get Wisp material
		wr::Material* material = material_manager.GetWispMaterial(material_handle);

		// Get plug name that has changed value
		MString changedPlugName = plug.partialName(false, false, false, false, false, true);

		// Apply changes to material
		MFnDependencyNode fn_dep_material(node);
		auto shader_type = material_parser->GetShaderType(node);
		switch (shader_type)
		{
			case wmr::detail::SurfaceShaderType::LAMBERT:
			{
				material_parser->HandleLambertChange(fn_dep_material, plug, changedPlugName, *material);
				break;
			}
			case wmr::detail::SurfaceShaderType::PHONG:
			{
				material_parser->HandlePhongChange(fn_dep_material, plug, changedPlugName, *material);
				break;
			}
			case wmr::detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER:
			{
				// CHANGE ARNOLD SHADER
				break;
			}
		}
	}
} /* namespace wmr */

void wmr::MaterialParser::ParseShadingEngineToWispMaterial(MObject & shading_engine, std::optional<MObject> fnmesh)
{
	auto& material_manager = m_renderer.GetMaterialManager();
	auto& texture_manager = m_renderer.GetTextureManager();

	// Get shader plug from shading engine
	auto opt_surface_shader_plug = GetSurfaceShader(shading_engine);
	if (!opt_surface_shader_plug.has_value())
		return;
	auto surface_shader_plug = opt_surface_shader_plug.value();

	// Get the actual surface shader plug
	auto opt_actual_surface_shader = GetActualSurfaceShaderPlug(surface_shader_plug);
	if (!opt_surface_shader_plug.has_value())
		return;
	// Node
	auto actual_surface_shader = opt_actual_surface_shader.value();
	auto actual_surface_shader_object = opt_actual_surface_shader.value().node();

	// Check if shader type not supported by this plug-in
	auto shader_type = GetShaderType(actual_surface_shader_object);
	if (shader_type == detail::SurfaceShaderType::UNSUPPORTED)
		return;

	// Wisp material
	wr::MaterialHandle material_handle = material_manager.CreateMaterial(fnmesh.value(), shading_engine, surface_shader_plug);
	
	InitialMaterialBuild(actual_surface_shader, shader_type, material_handle, material_manager, texture_manager);
}

void wmr::MaterialParser::InitialMaterialBuild(MPlug & surface_shader, detail::SurfaceShaderType shader_type, wr::MaterialHandle material_handle, MaterialManager & material_manager, TextureManager & texture_manager)
{
	// Get a Wisp material for this handle
	auto material = material_manager.GetWispMaterial(material_handle);
	MObject surface_shader_object = surface_shader.node();

	// Arnold PBR standard surface shader
	if (shader_type == detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER)
	{
		auto data = ParseArnoldStandardSurfaceShaderData(surface_shader_object);

		ConfigureWispMaterial(data, material, texture_manager);
	}

	// Found a Lambert shader
	if (shader_type == detail::SurfaceShaderType::LAMBERT)
	{

		// Retrieve the texture associated with this plug
		auto color_plug = GetPlugByName(surface_shader_object, MayaMaterialProps::plug_color);
		auto albedo_texture_path = GetPlugTexture(color_plug);

		// If there is no color available, use the RGBA values
		if (!albedo_texture_path.has_value())
		{

			MFnDependencyNode dep_node_fn(surface_shader.node());
			MString color_str(MayaMaterialProps::plug_color);
			MColor albedo_color = GetColor(dep_node_fn, color_str);

			material->SetConstantAlbedo({albedo_color.r, albedo_color.g, albedo_color.b});
			material->SetUseConstantAlbedo(true);
		}
		else
		{
			// Request new Wisp textures
			auto albedo_texture = texture_manager.CreateTexture(albedo_texture_path.value().asChar());

			// Use this texture as the material albedo texture
			material->SetAlbedo(*albedo_texture);
			material->SetUseConstantAlbedo(false);
		}
	}
}

// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::OnMeshAdded(MFnMesh& mesh)
{
	// Number of instances of this mesh
	std::uint32_t instance_count = mesh.parentCount();

	for (auto instance_index = 0; instance_index < instance_count; ++instance_index)
	{
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
				break;

				// All faces use the same material
			case 1:
				{
					auto shading_engine = shaders[0];

					MObject mesh_object = mesh.object();
					ParseShadingEngineToWispMaterial(shading_engine, mesh_object);
				}
				break;

				// Two or more materials are used (TODO)
			default:
				break;
		}
	}
}

void wmr::MaterialParser::OnCreateSurfaceShader(MPlug & surface_shader)
{
	wmr::SurfaceShaderShadingEngineRelation * relation = m_renderer.GetMaterialManager().OnCreateSurfaceShader(surface_shader);
	// When the surface shader already exists for some reason, don't do anything
	if (relation != nullptr)
	{

		// Add callback that filters on material changes
		MObject object = surface_shader.node();
		SubscribeSurfaceShader(object);

		// Check if shader type not supported by this plug-in
		auto shader_type = GetShaderType(surface_shader.node());
		if (shader_type == detail::SurfaceShaderType::UNSUPPORTED)
			return;

		auto& material_manager = m_renderer.GetMaterialManager();
		auto& texture_manager = m_renderer.GetTextureManager();
		InitialMaterialBuild(surface_shader, shader_type, relation->material_handle, material_manager, texture_manager);
	}
}

void wmr::MaterialParser::OnRemoveSurfaceShader(MPlug & surface_shader)
{
	// Call material manager on remove
	// Remove callback from material parser
}

void wmr::MaterialParser::ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().ConnectShaderToShadingEngine(surface_shader, shading_engine);
}

void wmr::MaterialParser::DisconnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().DisconnectShaderFromShadingEngine(surface_shader, shading_engine);
}

void wmr::MaterialParser::ConnectMeshToShadingEngine(MObject & mesh, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().ConnectMeshToShadingEngine(mesh, shading_engine);
}

void wmr::MaterialParser::DisconnectMeshFromShadingEngine(MObject & mesh, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().DisconnectMeshFromShadingEngine(mesh, shading_engine);
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
	{
		return shader_plug;
	}
	else
	{
		return std::nullopt;
	}
}

const std::optional<MPlug> wmr::MaterialParser::GetActualSurfaceShaderPlug(const MPlug & surface_shader_plug)
{
	MPlugArray connected_plugs;
	surface_shader_plug.connectedTo(connected_plugs, true, false);

	// Could not find a valid connected plug
	if (connected_plugs.length() <= 0)
		return std::nullopt;

	return connected_plugs[0];
}

void wmr::MaterialParser::SubscribeSurfaceShader(MObject & surface_shader)
{
	MaterialParser::ShaderDirtyData * data = new MaterialParser::ShaderDirtyData();
	data->material_parser = this;
	// Surface data is in this callback data, so I can figure out later what callback to remove (when a material is removed)
	data->surface_shader = surface_shader;

	MStatus status;
	MCallbackId addedId = MNodeMessage::addNodeDirtyCallback(
		surface_shader,
		DirtyNodeCallback,
		data,
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

void wmr::MaterialParser::ConfigureWispMaterial(const wmr::detail::ArnoldStandardSurfaceShaderData& data, wr::Material* material, TextureManager& texture_manager) const
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
		// Request new Wisp textures
		auto albedo_texture = texture_manager.CreateTexture(data.diffuse_color_texture_path);

		// Use this texture as the material albedo texture
		material->SetAlbedo(*albedo_texture);
	}

	if (data.using_specular_roughness_value)
	{
		material->SetConstantRoughness(data.diffuse_roughness);
	}
	else
	{
		// Request new Wisp textures
		auto roughness_texture = texture_manager.CreateTexture(data.specular_roughness_texture_path);

		// Use this texture as the material albedo texture
		material->SetRoughness(*roughness_texture);
	}

	if (data.using_metalness_value)
	{
		material->SetConstantMetallic({ data.metalness, 0.0f, 0.0f });
	}
	else
	{
		// Request new Wisp textures
		auto metalness_texture = texture_manager.CreateTexture(data.metalness_texture_path);

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
	auto diffuse_color_plug			= GetPlugByName(plug, detail::ArnoldStandardSurfaceShaderData::diffuse_color_plug_name);
	auto diffuse_roughness_plug		= GetPlugByName(plug, detail::ArnoldStandardSurfaceShaderData::diffuse_roughness_plug_name);
	auto metalness_plug				= GetPlugByName(plug, detail::ArnoldStandardSurfaceShaderData::metalness_plug_name);
	auto specular_color_plug		= GetPlugByName(plug, detail::ArnoldStandardSurfaceShaderData::specular_color_plug_name);
	auto specular_roughness_plug	= GetPlugByName(plug, detail::ArnoldStandardSurfaceShaderData::specular_roughness_plug_name);

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

void wmr::MaterialParser::HandleArnoldChange(MFnDependencyNode & fn, MPlug & plug, MString & plug_name, wr::Material & material)
{
	// CHANGE Arnold MATERIAL DATA
	// plug_name defines the attribute that is changed (e.g. "color" or "roughness")
}

const wmr::Renderer & wmr::MaterialParser::GetRenderer()
{
	return m_renderer;
}
