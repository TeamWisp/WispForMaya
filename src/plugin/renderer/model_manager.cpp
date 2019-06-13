#include "model_manager.hpp"

// Wisp plug-in
#include "miscellaneous/functions.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"
#include "settings.hpp"

// Wisp rendering framework
#include "d3d12/d3d12_renderer.hpp"

// Maya API
#include <maya/MViewport2Renderer.h>

void wmr::ModelManager::Initialize()
{
	auto* maya_override = dynamic_cast< const ViewportRendererOverride* >( MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME ) );
	m_model_pool = maya_override->GetRenderer().GetD3D12Renderer().CreateModelPool( settings::MAX_VERTEX_DATA_SIZE_MB, settings::MAX_INDEX_DATA_SIZE_MB ) ;
}

wr::Model* wmr::ModelManager::AddModel(const wr::MeshData<wr::Vertex>& data) noexcept
{
	// Load a model using the new model data
	auto model = m_model_pool->LoadCustom<wr::Vertex>({ data });

	// Just to avoid yet another call to "GetModelByName", the pointer is returned here already
	return model;
}

void wmr::ModelManager::UpdateModel(wr::Model& model, const wr::MeshData<wr::Vertex>& data )
{
	m_model_pool->EditMesh( model.m_meshes[0].first, data.m_vertices, data.m_indices.value() );
}

void wmr::ModelManager::Destroy() noexcept
{
	m_model_pool.reset();
}
