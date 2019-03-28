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
{
}

wmr::MaterialManager::~MaterialManager()
{
}

void wmr::MaterialManager::Initialize()
{
	auto& renderer = dynamic_cast< const ViewportRendererOverride* >( MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME ) )->GetRenderer();
	m_material_pool = renderer.GetD3D12Renderer().CreateMaterialPool( 0 );

	m_default_material_handle = m_material_pool->Create();

	wr::Material* internal_material = m_material_pool->GetMaterial( m_default_material_handle.m_id );
	auto& texture_manager = renderer.GetTextureManager();

	internal_material->UseNormalTexture( false );

	internal_material->SetUseConstantAlbedo( true );
	internal_material->SetUseConstantMetallic( true );
	internal_material->SetUseConstantRoughness( true );

	internal_material->SetConstantAlbedo( { 0.9f,0.9f,0.9f } );
	internal_material->SetConstantRoughness(  1.0f );
	internal_material->SetConstantMetallic( { 1.0f, 0.0f, 0.0f } );
}

wr::MaterialHandle wmr::MaterialManager::GetDefaultMaterial() noexcept
{
	return m_default_material_handle;
}

wr::MaterialHandle wmr::MaterialManager::CreateMaterial(MObject& fnmesh)
{
	MStatus status;
	wr::MaterialHandle material_handle = m_material_pool->Create();

	m_object_material_vector.push_back(std::make_pair(fnmesh, material_handle));

	// Assign material handle to all meshes of the mesh node
	if (m_scenegraph_parser == nullptr)
	{
		m_scenegraph_parser = &dynamic_cast<const ViewportRendererOverride*>(
			MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME)
			)->GetSceneGraphParser();
	}
	std::shared_ptr<wr::MeshNode> wr_mesh_node = m_scenegraph_parser->GetModelParser().GetWRModel(fnmesh);
	wr::Model* wr_model = wr_mesh_node->m_model;
	for (auto& m : wr_model->m_meshes)
	{
		m.second = material_handle;
	}

	return material_handle;
}

wr::MaterialHandle wmr::MaterialManager::DoesExist(MObject& object)
{
	wr::MaterialHandle handle;
	handle.m_pool = nullptr;

	for (auto& entry : m_object_material_vector)
	{
		if (entry.first == object)
		{
			return entry.second;
		}
	}
	return handle;
}

wr::Material * wmr::MaterialManager::GetMaterial(MObject & object)
{
	wr::MaterialHandle material_handle = DoesExist(object);
	if (material_handle.m_pool != nullptr)
	{
		return m_material_pool->GetMaterial(material_handle.m_id);
	}
	return nullptr;
}

wr::Material* wmr::MaterialManager::GetMaterial(wr::MaterialHandle handle) noexcept
{
	auto it = std::find_if(m_object_material_vector.begin(), m_object_material_vector.end(), [&handle](std::pair<MObject, wr::MaterialHandle> pair) {
		return (pair.second == handle);
	});

	auto end_it = --m_object_material_vector.end();

	if (it != end_it)
	{
		// Material does not exist!
		return nullptr;
	}

	// Retrieve the material from the pool and give it to the caller
	return m_material_pool->GetMaterial(handle.m_id);
}


