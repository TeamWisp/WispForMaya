#include "renderer.hpp"

//wisp plug-in
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/renderer/model_manager.hpp"

// Wisp rendering framework
#include "frame_graph/frame_graph.hpp"
#include "scene_graph/camera_node.hpp"
#include "scene_graph/scene_graph.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "wisp.hpp"

wmr::renderer::renderer()
{
}

wmr::renderer::~renderer()
{
}

void wmr::renderer::Initialize()
{
	
	m_render_system = std::make_unique<wr::D3D12RenderSystem>();
	m_window = std::make_unique<wr::Window>( GetModuleHandleA( nullptr ), "Wisp hidden window", 1280, 720 );
	m_render_system->Init( m_window.get() );


	// need managers
	m_model_manager = std::make_unique<ModelManager>();
	texture_pool = m_render_system->CreateTexturePool( 16, 14 );
	material_pool = m_render_system->CreateMaterialPool( 8 );


	m_scenegraph = std::make_shared<wr::SceneGraph>( m_render_system.get() );


	// multiple cameras??
	m_viewport_camera = m_scenegraph->CreateChild<wr::CameraNode>( nullptr, 90.f, ( float )m_window->GetWidth() / ( float )m_window->GetHeight() );
	m_viewport_camera->SetPosition( { 0, 0, -1 } );



	m_framegraph_manager = std::make_unique<FrameGraphManager>();
	m_framegraph_manager->Create( *m_render_system, RendererFrameGraphType::DEFERRED );
}

void wmr::renderer::Update()
{
	
}

void wmr::renderer::Render()
{
	m_render_system->Render(m_scenegraph , *m_framegraph_manager->Get());

}
