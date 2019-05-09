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
				material_parser->HandleArnoldChange(fn_dep_material, plug, changedPlugName, *material);
				break;
			}
		}

		material->UpdateConstantBuffer();
	}
} /* namespace wmr */

void wmr::MaterialParser::ParseShadingEngineToWispMaterial(MObject & shading_engine, MObject & fnmesh)
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
	wr::MaterialHandle material_handle = material_manager.CreateMaterial(fnmesh, shading_engine, actual_surface_shader);
	
	InitialMaterialBuild(actual_surface_shader, shader_type, material_handle, material_manager, texture_manager);
}

void wmr::MaterialParser::InitialMaterialBuild(MPlug & surface_shader, detail::SurfaceShaderType shader_type, wr::MaterialHandle material_handle, MaterialManager & material_manager, TextureManager & texture_manager)
{
	// Get a Wisp material for this handle
	auto material = material_manager.GetWispMaterial(material_handle);
	MObject surface_shader_object = surface_shader.node();

	// Subscribe the surface shader to listen for changes
	SubscribeSurfaceShader(surface_shader_object);

	// Found a Lambert shader
	if (shader_type == detail::SurfaceShaderType::LAMBERT)
	{
	auto data = ParseLambertShaderData(surface_shader_object);

	ConfigureWispMaterial(data, material, m_renderer.GetTextureManager());
	}
	// Found a Phong shader
	else if (shader_type == detail::SurfaceShaderType::PHONG)
	{
	auto data = ParsePhongShaderData(surface_shader_object);

	ConfigureWispMaterial(data, material, m_renderer.GetTextureManager());
	}
	// Arnold PBR standard surface shader
	else if (shader_type == detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER)
	{
		auto data = ParseArnoldStandardSurfaceShaderData(surface_shader_object);

		ConfigureWispMaterial(data, material, texture_manager);
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
	m_renderer.GetMaterialManager().OnRemoveSurfaceShader(surface_shader);

	// Remove callback from material parser
	MObject surface_shader_object = surface_shader.node();
	auto it = std::find_if(shader_dirty_datas.begin(), shader_dirty_datas.end(), [&surface_shader_object] (const std::vector<ShaderDirtyData*>::value_type& vt)
	{
		return (vt->surface_shader == surface_shader_object);
	});
	if (it != shader_dirty_datas.end())
	{
		ShaderDirtyData* data = *it;

		// Remove callback from callback manager
		CallbackManager::GetInstance().UnregisterCallback(data->callback_id);
		delete data;

		shader_dirty_datas.erase(it);
	}
}

void wmr::MaterialParser::ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	m_renderer.GetMaterialManager().ConnectShaderToShadingEngine(surface_shader, shading_engine, true);

	// Find if shader has callback
	MObject surface_shader_object = surface_shader.node();
	SubscribeSurfaceShader(surface_shader_object);
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

	if (shader_type_name == MayaMaterialProps::maya_lambert_shader_name)
	{
		return detail::SurfaceShaderType::LAMBERT;
	}
	else if (shader_type_name == MayaMaterialProps::maya_phong_shader_name)
	{
		return detail::SurfaceShaderType::PHONG;
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
	// Find surface shader change callback
	auto it = std::find_if(shader_dirty_datas.begin(), shader_dirty_datas.end(), [&surface_shader] (const std::vector<ShaderDirtyData*>::value_type& vt)
	{
		return (vt->surface_shader == surface_shader);
	});
	// Add new callback if the surface shader doesn't have a callback yet.
	if (it == shader_dirty_datas.end())
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
}

wmr::LambertShaderData wmr::MaterialParser::ParseLambertShaderData(const MObject & plug)
{
	LambertShaderData data = {};

	// Need this to get access to functions that allow us to retrieve the data from the plug
	MFnDependencyNode dep_node_fn(plug);

	// Get all PBR variables
	auto diffuse_color_plug = GetPlugByName(plug, LambertShaderData::diffuse_color_plug_name);
	auto bump_map_plug = GetPlugByName(plug, LambertShaderData::bump_map_plug_name);

	// Attempt to retrieve a texture for each PBR variable
	auto diffuse_color_texture_path = GetPlugTexture(diffuse_color_plug);
	auto bump_map_texture_path = GetPlugTexture(bump_map_plug);

	// Diffuse color
	if (diffuse_color_texture_path.has_value())
	{
		data.using_diffuse_color_value = false;
		data.diffuse_color_texture_path = diffuse_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = LambertShaderData::diffuse_color_plug_name;

		auto color = GetColor(dep_node_fn, plug_name);

		data.diffuse_color[0] = color.r;
		data.diffuse_color[1] = color.g;
		data.diffuse_color[2] = color.b;
	}

	// Bump map
	if (bump_map_texture_path.has_value())
	{
		data.bump_map_texture_path = bump_map_texture_path.value().asChar();
	}

	return data;
}

wmr::PhongShaderData wmr::MaterialParser::ParsePhongShaderData(const MObject & plug)
{
	PhongShaderData data = {};

	// Need this to get access to functions that allow us to retrieve the data from the plug
	MFnDependencyNode dep_node_fn(plug);

	// Get all PBR variables
	auto diffuse_color_plug = GetPlugByName(plug, PhongShaderData::diffuse_color_plug_name);
	auto bump_map_plug = GetPlugByName(plug, PhongShaderData::bump_map_plug_name);

	// Attempt to retrieve a texture for each PBR variable
	auto diffuse_color_texture_path = GetPlugTexture(diffuse_color_plug);
	auto bump_map_texture_path = GetPlugTexture(bump_map_plug);

	// Diffuse color
	if (diffuse_color_texture_path.has_value())
	{
		data.using_diffuse_color_value = false;
		data.diffuse_color_texture_path = diffuse_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = PhongShaderData::diffuse_color_plug_name;

		auto color = GetColor(dep_node_fn, plug_name);

		data.diffuse_color[0] = color.r;
		data.diffuse_color[1] = color.g;
		data.diffuse_color[2] = color.b;
	}

	// Bump map
	if (bump_map_texture_path.has_value())
	{
		data.bump_map_texture_path = bump_map_texture_path.value().asChar();
	}

	return data;
}

wmr::ArnoldStandardSurfaceShaderData wmr::MaterialParser::ParseArnoldStandardSurfaceShaderData(const MObject& plug)
{
	ArnoldStandardSurfaceShaderData data = {};

	// Need this to get access to functions that allow us to retrieve the data from the plug
	MFnDependencyNode dep_node_fn(plug);

	// Get all PBR variables
	auto diffuse_color_plug			= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::diffuse_color_plug_name);
	auto diffuse_roughness_plug		= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::diffuse_roughness_plug_name);
	auto metalness_plug				= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::metalness_plug_name);
	auto specular_color_plug		= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::specular_color_plug_name);
	auto specular_roughness_plug	= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::specular_roughness_plug_name);
	auto bump_map_plug				= GetPlugByName(plug,ArnoldStandardSurfaceShaderData::bump_map_plug_name);

	// Attempt to retrieve a texture for each PBR variable
	auto diffuse_color_texture_path			= GetPlugTexture(diffuse_color_plug);
	auto diffuse_roughness_texture_path		= GetPlugTexture(diffuse_roughness_plug);
	auto metalness_texture_path				= GetPlugTexture(metalness_plug);
	auto specular_color_texture_path		= GetPlugTexture(specular_color_plug);
	auto specular_roughness_texture_path	= GetPlugTexture(specular_roughness_plug);
	auto bump_map_texture_path				= GetPlugTexture(bump_map_plug);

	// Diffuse color
	if (diffuse_color_texture_path.has_value())
	{
		data.using_diffuse_color_value = false;
		data.diffuse_color_texture_path = diffuse_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = ArnoldStandardSurfaceShaderData::diffuse_color_plug_name;

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
		dep_node_fn.findPlug(ArnoldStandardSurfaceShaderData::diffuse_roughness_plug_name).getValue(data.diffuse_roughness);
	}

	// Metalness
	if (metalness_texture_path.has_value())
	{
		data.using_metalness_value = false;
		data.metalness_texture_path = metalness_texture_path.value().asChar();
	}
	else
	{
		dep_node_fn.findPlug(ArnoldStandardSurfaceShaderData::metalness_plug_name).getValue(data.metalness);
	}

	// Specular color
	if (specular_color_texture_path.has_value())
	{
		data.using_specular_color_value = false;
		data.specular_color_texture_path = specular_color_texture_path.value().asChar();
	}
	else
	{
		MString plug_name = ArnoldStandardSurfaceShaderData::specular_color_plug_name;

		auto color = GetColor(dep_node_fn, plug_name);

		data.specular_color[0] = color.r;
		data.specular_color[1] = color.g;
		data.specular_color[2] = color.b;
	}

	// Specular roughness
	if (specular_roughness_texture_path.has_value())
	{
		data.using_specular_roughness_value = false;
		data.specular_roughness_texture_path = specular_roughness_texture_path.value().asChar();
	}
	else
	{
		dep_node_fn.findPlug(ArnoldStandardSurfaceShaderData::specular_roughness_plug_name).getValue(data.specular_roughness);
	}

	// Bump map
	if (bump_map_texture_path.has_value())
	{
		data.bump_map_texture_path = bump_map_texture_path.value().asChar();
	}

	return data;
}

void wmr::MaterialParser::ConfigureWispMaterial(const wmr::LambertShaderData & data, wr::Material * material, TextureManager & texture_manager) const
{
	if (data.using_diffuse_color_value)
	{
		material->SetConstant<wr::MaterialConstant::COLOR>({ data.diffuse_color[0], data.diffuse_color[1], data.diffuse_color[2] });
	}
	else
	{
		// Request new Wisp textures
		auto albedo_texture = texture_manager.CreateTexture(data.diffuse_color_texture_path);

		// Use this texture as the material albedo texture
		material->SetTexture(wr::TextureType::ALBEDO, *albedo_texture);
	}

	if (strcmp(data.bump_map_texture_path, "") != 0)
	{
		auto bump = texture_manager.CreateTexture(data.bump_map_texture_path);
		material->SetTexture(wr::TextureType::NORMAL, *bump);
	}

	material->SetConstant<wr::MaterialConstant::METALLIC>(MayaMaterialProps::default_metallicness);
	material->SetConstant<wr::MaterialConstant::ROUGHNESS>(MayaMaterialProps::default_roughness);
}

void wmr::MaterialParser::ConfigureWispMaterial(const wmr::PhongShaderData & data, wr::Material * material, TextureManager & texture_manager) const
{
	if (data.using_diffuse_color_value)
	{
		material->SetConstant<wr::MaterialConstant::COLOR>({ data.diffuse_color[0], data.diffuse_color[1], data.diffuse_color[2] });
	}
	else
	{
		// Request new Wisp textures
		auto albedo_texture = texture_manager.CreateTexture(data.diffuse_color_texture_path);

		// Use this texture as the material albedo texture
		material->SetTexture(wr::TextureType::ALBEDO, *albedo_texture);
	}

	if (strcmp(data.bump_map_texture_path, "") != 0)
	{
		auto bump = texture_manager.CreateTexture(data.bump_map_texture_path);
		material->SetTexture(wr::TextureType::NORMAL, *bump);
	}

	material->SetConstant<wr::MaterialConstant::METALLIC>(MayaMaterialProps::default_metallicness);
	material->SetConstant<wr::MaterialConstant::ROUGHNESS>(MayaMaterialProps::default_roughness);
}

void wmr::MaterialParser::ConfigureWispMaterial(const wmr::ArnoldStandardSurfaceShaderData& data, wr::Material* material, TextureManager& texture_manager) const
{
	if (data.using_diffuse_color_value)
	{
		material->SetConstant<wr::MaterialConstant::COLOR>({ data.diffuse_color[0], data.diffuse_color[1], data.diffuse_color[2] });
	}
	else
	{
		// Request new Wisp textures
		auto albedo_texture = texture_manager.CreateTexture(data.diffuse_color_texture_path);

		// Use this texture as the material albedo texture
		material->SetTexture(wr::TextureType::ALBEDO, *albedo_texture);
	}

	if (data.using_specular_roughness_value)
	{
		material->SetConstant<wr::MaterialConstant::ROUGHNESS>(data.diffuse_roughness);
	}
	else
	{
		// Request new Wisp textures
		auto roughness_texture = texture_manager.CreateTexture(data.specular_roughness_texture_path);

		// Use this texture as the material roughness texture
		material->SetTexture(wr::TextureType::ROUGHNESS, *roughness_texture);
	}

	if (data.using_metalness_value)
	{
		material->SetConstant<wr::MaterialConstant::METALLIC>(data.metalness);
	}
	else
	{
		// Request new Wisp textures
		auto metalness_texture = texture_manager.CreateTexture(data.metalness_texture_path);

		// Use this texture as the material albedo texture
		material->SetTexture(wr::TextureType::METALLIC, *metalness_texture);
	}

	if (strcmp(data.bump_map_texture_path, "") != 0)
	{
		auto bump = texture_manager.CreateTexture(data.bump_map_texture_path);
		material->SetTexture(wr::TextureType::NORMAL, *bump);
	}
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
	MObject plug_object = plug.node();
	auto data = ParseLambertShaderData(plug_object);

	ConfigureWispMaterial(data, &material, m_renderer.GetTextureManager());
}

void wmr::MaterialParser::HandlePhongChange(MFnDependencyNode & fn, MPlug & plug, MString & plug_name, wr::Material & material)
{
	MObject plug_object = plug.node();
	auto data = ParsePhongShaderData(plug_object);

	ConfigureWispMaterial(data, &material, m_renderer.GetTextureManager());
}

void wmr::MaterialParser::HandleArnoldChange(MFnDependencyNode & fn, MPlug & plug, MString & plug_name, wr::Material & material)
{
	// CHANGE Arnold MATERIAL DATA
	MObject plug_object = plug.node();
	auto data = ParseArnoldStandardSurfaceShaderData(plug_object);

	ConfigureWispMaterial(data, &material, m_renderer.GetTextureManager());
}

const wmr::Renderer & wmr::MaterialParser::GetRenderer()
{
	return m_renderer;
}
