#include "material_manager.hpp"

#include "plugin/parsers/model_parser.hpp"
#include "plugin/parsers/scene_graph_parser.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "miscellaneous/settings.hpp"

#include "d3d12/d3d12_renderer.hpp"
#include "scene_graph/mesh_node.hpp"

#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MViewport2Renderer.h>


wmr::MaterialManager::MaterialManager() :
	m_scenegraph_parser(nullptr)
{ }

wmr::MaterialManager::~MaterialManager()
{ }

void wmr::MaterialManager::Initialize()
{
	auto& renderer = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer();
	m_material_pool = renderer.GetD3D12Renderer().CreateMaterialPool(0);

	m_default_material_handle = m_material_pool->Create();

	wr::Material* internal_material = m_material_pool->GetMaterial(m_default_material_handle.m_id);
	auto& texture_manager = renderer.GetTextureManager();

	internal_material->SetAlbedo(texture_manager.GetDefaultTexture());
	internal_material->SetNormal(texture_manager.GetDefaultTexture());
	internal_material->SetMetallic(texture_manager.GetDefaultTexture());
	internal_material->SetRoughness(texture_manager.GetDefaultTexture());
}

void wmr::MaterialManager::DisconnectMeshFromShadingEngine(MFnMesh & fnmesh, MObject & shading_engine)
{ }

wr::MaterialHandle wmr::MaterialManager::GetDefaultMaterial() noexcept
{
	return m_default_material_handle;
}

wr::MaterialHandle wmr::MaterialManager::CreateMaterial(MObject& fnmesh, MObject &shading_engine, MPlug &surface_shader)
{
	MStatus status;
	// Add mesh and shading engine relationship
	m_mesh_shading_relations.push_back({
		fnmesh,				// mesh
		shading_engine		// shadingEngine
									   });

	// Add shading engine relationship
	wr::MaterialHandle material_handle;
	SurfaceShaderShadingEngineRelation *relation = DoesSurfaceShaderExist(surface_shader);
	// Create new relationship the surface shader doesn't exist
	if (relation == nullptr)
	{
		// Create Wisp Material handle
		material_handle = m_material_pool->Create();
		// Create a vector for the shading engines
		std::vector<MObject> shading_engines;
		shading_engines.push_back(shading_engine);
		// Create relationship between surface shader and shading engine
		m_surface_shader_shading_relations.push_back({
			material_handle,		// Wisp Material handle
			surface_shader,			// Maya surface shader plug
			shading_engines			// Vector of shading engines
													 });
	}
	// Get material handle from existing relationship 
	else
	{
		material_handle = relation->material_handle;
	}

	// Assign material handle to all meshes of the mesh node
	ApplyMaterialToModel(material_handle, fnmesh);

	return material_handle;
}

wr::MaterialHandle wmr::MaterialManager::ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	auto relation = DoesSurfaceShaderExist(surface_shader);
	if (relation != nullptr)
	{
		auto shading_engine_it = relation->FindShadingEngine(shading_engine);
		if (shading_engine_it == relation->shading_engines.end())
		{
			relation->shading_engines.push_back(shading_engine);
		}
		return relation->material_handle;
	}
	// Surface shader doesn't have a material assigned to it yet
	// Create Wisp Material handle
	wr::MaterialHandle material_handle = m_material_pool->Create();
	// Create a vector for the shading engines
	std::vector<MObject> shading_engines;
	shading_engines.push_back(shading_engine);
	// Create relationship between surface shader and shading engine
	m_surface_shader_shading_relations.push_back({
		material_handle,		// Wisp Material handle
		surface_shader,			// Maya surface shader plug
		shading_engines			// Vector of shading engines
	});

	return material_handle;
}

void wmr::MaterialManager::DisonnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{ }

void wmr::MaterialManager::ConnectMeshToShadingEngine(MFnMesh & fnmesh, MObject & shading_engine)
{ }

wr::Material * wmr::MaterialManager::GetWispMaterial(wr::MaterialHandle & material_handle)
{
	return m_material_pool->GetMaterial(material_handle.m_id);
}

wmr::SurfaceShaderShadingEngineRelation * wmr::MaterialManager::DoesMaterialHandleExist(wr::MaterialHandle & material_handle)
{
	// Search relationships for material handle
	for (auto& relation : m_surface_shader_shading_relations)
	{
		if (relation.material_handle == material_handle)
		{
			return &relation;
		}
	}
	return nullptr;
}

wmr::SurfaceShaderShadingEngineRelation * wmr::MaterialManager::DoesSurfaceShaderExist(MPlug & surface_shader)
{
	// Search relationships for shading engines
	for (auto& relation : m_surface_shader_shading_relations)
	{
		if (relation.surface_shader == surface_shader)
		{
			return &relation;
		}
	}
	return nullptr;
}

wmr::ScenegraphParser * wmr::MaterialManager::GetSceneParser()
{
	if (m_scenegraph_parser == nullptr)
	{
		m_scenegraph_parser = &dynamic_cast<const ViewportRendererOverride*>(
			MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME)
			)->GetSceneGraphParser();
	}
	return m_scenegraph_parser;
}

void wmr::MaterialManager::ApplyMaterialToModel(wr::MaterialHandle material_handle, MObject & fnmesh)
{
	
	std::shared_ptr<wr::MeshNode> wr_mesh_node = GetSceneParser()->GetModelParser().GetWRModel(fnmesh);
	wr::Model* wr_model = wr_mesh_node->m_model;
	for (auto& mesh : wr_model->m_meshes)
	{
		mesh.second = material_handle;
	}
}

std::vector<MObject>::iterator wmr::SurfaceShaderShadingEngineRelation::FindShadingEngine(MObject & shading_engine)
{
	auto it = std::find_if(shading_engines.begin(), shading_engines.end(), [&shading_engine] (const std::vector<MObject>::value_type& vt)
	{
		return (vt == shading_engine);
	});
	return it;
}
