#include "renderer.hpp"

wmr::renderer::renderer()
{
}

wmr::renderer::~renderer()
{
}

void wmr::renderer::Initialize()
{
	util::log_callback::impl = [ & ]( std::string const & str )
	{
		engine::debug_console.AddLog( str.c_str() );
	};

	m_render_system = std::make_unique<wr::D3D12RenderSystem>();

	m_model_loader = std::make_unique<wr::AssimpModelLoader>();

	m_render_system->Init( window.get() );

	texture_pool = m_render_system->CreateTexturePool( 16, 14 );
	material_pool = m_render_system->CreateMaterialPool( 8 );


	m_scenegraph = std::make_shared<wr::SceneGraph>( m_render_system.get() );

	m_viewport_camera = m_scenegraph->CreateChild<wr::CameraNode>( nullptr, 90.f, ( float )window->GetWidth() / ( float )window->GetHeight() );
	m_viewport_camera->SetPosition( { 0, 0, -1 } );

	m_frame_graph_manager = std::make_unique<FrameGraphManager>();
	m_frame_graph_manager->Create( *m_render_system, RendererFrameGraphType::DEFERRED );
}

void wmr::renderer::Update()
{
}

void wmr::renderer::Render()
{
}
