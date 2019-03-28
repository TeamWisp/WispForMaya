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
		// Get material parser from client data
		wmr::MaterialParser *material_parser = reinterpret_cast<wmr::MaterialParser*>(clientData);
		
		// Get Maya mesh object that is using the Maya material
		std::optional<MObject> mesh_object = material_parser->GetMeshObjectFromMaterial(node);
		if (!mesh_object.has_value())
		{
			MGlobal::displayError("A connect to a material could not be found! (wmr::DirtyNodeCallback)");
			return;
		}

		// Get Wisp material that is used by the found Maya mesh object
		wr::Material *material = material_parser->GetRenderer().GetMaterialManager().GetMaterial(mesh_object.value());

		// Get plug name that has changed value
		MString changedPlugName = plug.partialName(false, false, false, false, false, true);

		MFnDependencyNode fn_material(node);
		if (material != nullptr)
		{
			if (node.hasFn(MFn::kLambert))
			{
				material_parser->HandleLambertChange(fn_material, plug, changedPlugName, *material);
			}
			else if (node.hasFn(MFn::kPhong))
			{
				material_parser->HandlePhongChange(fn_material, plug, changedPlugName, *material);
			}
		}
	}
} /* namespace wmr */



// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::Parse(const MFnMesh& mesh)
{
	auto& material_manager = m_renderer.GetMaterialManager();
	auto& texture_manager = m_renderer.GetTextureManager();

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
				break;

				// All faces use the same material
			case 1:
				ParseSingleSurfaceShader(shaders[0], mesh, material_manager, texture_manager);
				break;

				// Two or more materials are used (TODO)
			default:
				break;
		}
	}
}

void wmr::MaterialParser::ParseSingleSurfaceShader(const MObject& node, const MFnMesh& mesh, MaterialManager& material_manager, TextureManager& texture_manager)
{
	auto surface_shader = GetSurfaceShader(node);

	// Invalid surface shader
	if (!surface_shader.has_value())
	{
		return;
	}

	// Find all plugs that are connected to this shader
	MPlugArray connected_plugs;
	surface_shader.value().connectedTo(connected_plugs, true, false);

	// Could not find a valid connected plug
	if (connected_plugs.length() != 1)
	{
		return;
	}

	MObject connected_plug = connected_plugs[0].node();

	auto shader_type = GetShaderType(connected_plug);

	// Shader type not supported by this plug-in
	if (shader_type == detail::SurfaceShaderType::UNSUPPORTED)
	{
		return;
	}

	// Wisp material
	MObject object = mesh.object();
	wr::MaterialHandle material_handle = material_manager.DoesExist(object);

	// Create a new material if none exists yet
	if (!material_handle.m_pool)
	{
		material_handle = material_manager.CreateMaterial(object);
	}

	// Get a Wisp material for this handle
	auto material = material_manager.GetMaterial(material_handle);

	// Arnold PBR standard surface shader
	if (shader_type == detail::SurfaceShaderType::ARNOLD_STANDARD_SURFACE_SHADER)
	{
		auto data = ParseArnoldStandardSurfaceShaderData(connected_plug);

		mesh_material_relations.push_back(std::make_pair(connected_plug, object));

		MStatus status;
		MCallbackId attributeId = MNodeMessage::addNodeDirtyCallback(
			connected_plug,
			DirtyNodeCallback,
			this,
			&status
		);

		CallbackManager::GetInstance().RegisterCallback(attributeId);

		ConfigureWispMaterial(data, material, texture_manager);
	}

	// Found a Lambert shader
	if (shader_type == detail::SurfaceShaderType::LAMBERT)
	{
		auto color_plug = GetPlugByName(connected_plug, MayaMaterialProps::plug_color);

		// Retrieve the texture associated with this plug
		auto albedo_texture_path = GetPlugTexture(color_plug);

		mesh_material_relations.push_back(std::make_pair(connected_plug, object));

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
		if (!albedo_texture_path.has_value())
		{

			MFnDependencyNode dep_node_fn(connected_plug);
			MString color_str(MayaMaterialProps::plug_color);
			MColor albedo_color = GetColor(dep_node_fn, color_str);

			material->SetConstantAlbedo({ albedo_color.r, albedo_color.g, albedo_color.b });
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

const wmr::Renderer& wmr::MaterialParser::GetRenderer()
{
	return m_renderer;
}
