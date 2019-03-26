#include "material_manager.hpp"

#include "plugin/parsers/model_parser.hpp"
#include "plugin/parsers/scene_graph_parser.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "miscellaneous/settings.hpp"

#include "d3d12/d3d12_renderer.hpp"
#include "scene_graph/mesh_node.hpp"

#include <maya/MFnMesh.h>
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

wr::MaterialHandle wmr::MaterialManager::GetDefaultMaterial() noexcept
{
	return m_default_material_handle;
}

wr::MaterialHandle wmr::MaterialManager::CreateMaterial(MObject& fnmesh, MObject &shading_engine, MPlug &surface_shader)
{
	MStatus status;
	MFnMesh mesh(fnmesh);

	wr::MaterialHandle material_handle = ConnectShaderToShadingEngine(surface_shader, shading_engine);

	ConnectMeshToShadingEngine(mesh, shading_engine);

	return material_handle;
}

wr::MaterialHandle wmr::MaterialManager::ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{
	// Find surface shader relationships
	auto relation = DoesSurfaceShaderExist(surface_shader);
	if (relation != nullptr)
	{
		// Add shading engine if surface shader doesn't have a relation with the shading engine
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

void wmr::MaterialManager::DisconnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine)
{ 
	auto relation = DoesSurfaceShaderExist(surface_shader);
	if (relation != nullptr)
	{
		auto shading_engine_it = relation->FindShadingEngine(shading_engine);
		if (shading_engine_it != relation->shading_engines.end())
		{
			relation->shading_engines.erase(shading_engine_it);
		}
	}
}

void wmr::MaterialManager::ConnectMeshToShadingEngine(MFnMesh & fnmesh, MObject & shading_engine)
{
	MObject mesh_obj = fnmesh.object();
	auto it = std::find_if(m_mesh_shading_relations.begin(), m_mesh_shading_relations.end(), [&mesh_obj] (const std::vector<MeshShadingEngineRelation>::value_type& vt)
	{
		return (vt.mesh == mesh_obj);
	});
	// If the mesh already has a shading engine
	if (it != m_mesh_shading_relations.end())
	{
		it->shading_engine = shading_engine;
	}
	// If the mesh was not found
	else
	{
		MeshShadingEngineRelation newRelation;
		newRelation.mesh = mesh_obj;
		newRelation.shading_engine = shading_engine;
		m_mesh_shading_relations.push_back(newRelation);
	}

	wr::MaterialHandle material_handle = FindWispMaterialByShadingEngine(shading_engine);
	ApplyMaterialToModel(material_handle, mesh_obj);
}

void wmr::MaterialManager::DisconnectMeshFromShadingEngine(MFnMesh & fnmesh, MObject & shading_engine, bool reset_material)
{
	MObject mesh_obj = fnmesh.object();
	auto it = std::find_if(m_mesh_shading_relations.begin(), m_mesh_shading_relations.end(), [&mesh_obj, &shading_engine] (const std::vector<MeshShadingEngineRelation>::value_type& vt)
	{
		return (vt.mesh == mesh_obj && vt.shading_engine == shading_engine);
	});
	// Found the relation between the two given parameters (mesh and shading engine)
	if (it != m_mesh_shading_relations.end())
	{
		m_mesh_shading_relations.erase(it);

		if (reset_material)
		{
			MObject mesh = fnmesh.object();
			ApplyMaterialToModel(m_default_material_handle, mesh);
		}
	}
}

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

wr::MaterialHandle wmr::MaterialManager::FindWispMaterialByShadingEngine(MObject & shading_engine)
{
	auto it = std::find_if(m_surface_shader_shading_relations.begin(), m_surface_shader_shading_relations.end(), [&shading_engine] (const std::vector<SurfaceShaderShadingEngineRelation>::value_type& vt)
	{
		auto end_it = vt.shading_engines.end();
		auto shading_engines_it = std::find_if(vt.shading_engines.begin(), end_it, [&shading_engine] (const std::vector<MObject>::value_type& vt)
		{
			return (vt == shading_engine);
		});

		return (shading_engines_it != end_it);
	});

	if (it != m_surface_shader_shading_relations.end())
	{
		return it->material_handle;
	}
	return m_default_material_handle;
}

void wmr::MaterialManager::ApplyMaterialToModel(wr::MaterialHandle & material_handle, MObject & fnmesh)
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
