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
{
	m_render_system = std::make_unique<wr::D3D12RenderSystem>();
	m_window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "Wisp hidden window", 1280, 720);
	m_render_system->Init(m_window.get());

	m_model_manager = std::make_unique<ModelManager>();
	m_texture_manager = std::make_unique<TextureManager>();
	m_material_manager = std::make_unique<MaterialManager>();

	m_scenegraph = std::make_shared<wr::SceneGraph>(m_render_system.get());

	m_wisp_camera = m_scenegraph->CreateChild<wr::CameraNode>(nullptr, 90.f, (float)m_window->GetWidth() / (float)m_window->GetHeight());
	m_wisp_camera->SetPosition({ 0, 0, -1 });

	m_render_system->InitSceneGraph(*m_scenegraph.get());
	m_framegraph_manager = std::make_unique<FrameGraphManager>();
	m_framegraph_manager->Create(*m_render_system, RendererFrameGraphType::DEFERRED);
}

wmr::Renderer::~Renderer()
{
	m_render_system->WaitForAllPreviousWork();
	m_render_system.reset();
}

void wmr::Renderer::Initialize() noexcept
{
	m_texture_manager->Initialize();
}

void wmr::Renderer::Update()
{
	
}

void wmr::Renderer::Render()
{
	m_render_system->Render(m_scenegraph , *m_framegraph_manager->Get());

}

const wr::CPUTextures wmr::Renderer::GetRenderResult()
{
	return m_result_textures;
}

wmr::ModelManager & wmr::Renderer::GetModelManager() const
{
	return *m_model_manager.get();
}

wmr::FrameGraphManager & wmr::Renderer::GetFrameGraph() const
{
	return *m_framegraph_manager.get();
}

wmr::MaterialManager & wmr::Renderer::GetMaterialManager() const
{
	return *m_material_manager.get();
}

wmr::TextureManager & wmr::Renderer::GetTextureManager() const
{
	return *m_texture_manager.get();
}

wr::SceneGraph & wmr::Renderer::GetScenegraph() const
{
	return *m_scenegraph;
}

wr::D3D12RenderSystem & wmr::Renderer::GetD3D12Renderer() const
{
	return *m_render_system.get();
}
