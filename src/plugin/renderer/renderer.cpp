// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zijnstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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

#include "renderer.hpp"

//wisp plug-in
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/renderer/model_manager.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "plugin/renderer/material_manager.hpp"

// Wisp rendering framework
#include "frame_graph/frame_graph.hpp"
#include "scene_graph/camera_node.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "wisp.hpp"

wmr::Renderer::Renderer()
	: m_frame_index(0)
{
	LOG("Starting object creation.");

	m_render_system			= std::make_unique<wr::D3D12RenderSystem>();
	m_window				= std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "Wisp hidden window", 1280, 720, false);
	m_model_manager			= std::make_unique<ModelManager>();
	m_texture_manager		= std::make_unique<TextureManager>(this);
	m_material_manager		= std::make_unique<MaterialManager>();
	m_framegraph_manager	= std::make_unique<FrameGraphManager>();

	m_scenegraph			= std::make_shared<wr::SceneGraph>(m_render_system.get());

	LOG("Finished object creation.");
}

wmr::Renderer::~Renderer()
{}

void wmr::Renderer::Initialize() noexcept
{
	LOG("Starting renderer initialization.");
	m_render_system	->Init(m_window.get());

	m_model_manager->Initialize();
	m_texture_manager->Initialize();
	m_material_manager->Initialize();

	m_wisp_camera = m_scenegraph->CreateChild<wr::CameraNode>(nullptr, (float)m_window->GetWidth() / (float)m_window->GetHeight());
	m_wisp_camera->SetPosition({ 0, 0, -1 });
	m_wisp_camera->m_override_projection = true;

	auto skybox = m_scenegraph->CreateChild<wr::SkyboxNode>(nullptr, m_texture_manager->GetDefaultSkybox());

	m_render_system->InitSceneGraph(*m_scenegraph);
	m_framegraph_manager->Create(*m_render_system, RendererFrameGraphType::DEFERRED);
	LOG("Finished renderer initialization.");
}

void wmr::Renderer::Update()
{
	m_frame_index = m_render_system->GetFrameIdx();
}

void wmr::Renderer::Render()
{
	m_result_textures = m_render_system->Render(*m_scenegraph , *m_framegraph_manager->Get());
}

void wmr::Renderer::Destroy()
{
	m_model_manager->Destroy();
	m_texture_manager->Destroy();
	m_material_manager->Destroy();
	m_framegraph_manager->Destroy();
	m_render_system.reset();
	m_window->Stop();
}

void wmr::Renderer::UpdateSkybox(const std::string& path) noexcept
{
	m_scenegraph->UpdateSkyboxNode(m_scenegraph->GetCurrentSkybox(), *m_texture_manager->CreateTexture(path.c_str()));
}

const wr::CPUTextures wmr::Renderer::GetRenderResult()
{
	return m_result_textures;
}

const std::uint64_t wmr::Renderer::GetFrameIndex() const noexcept(true)
{
	return m_frame_index;
}

wmr::ModelManager& wmr::Renderer::GetModelManager() const
{
	return *m_model_manager;
}

wmr::FrameGraphManager& wmr::Renderer::GetFrameGraph() const
{
	return *m_framegraph_manager;
}

wmr::MaterialManager& wmr::Renderer::GetMaterialManager() const
{
	return *m_material_manager;
}

wmr::TextureManager& wmr::Renderer::GetTextureManager() const
{
	return *m_texture_manager;
}

wr::SceneGraph& wmr::Renderer::GetScenegraph() const
{
	return *m_scenegraph;
}

wr::D3D12RenderSystem& wmr::Renderer::GetD3D12Renderer() const
{
	return *m_render_system;
}

std::shared_ptr<wr::CameraNode> wmr::Renderer::GetCamera() const
{
	return m_wisp_camera;
}
