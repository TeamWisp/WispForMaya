// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
