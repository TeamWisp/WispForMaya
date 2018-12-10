#include "Renderer.hpp"

#include "d3d12/d3d12_renderer.hpp"
#include "frame_graph/frame_graph.hpp"

namespace wmr::wri
{
	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}
	
	void Renderer::Initialize(unsigned int render_target_width, unsigned int render_target_height)
	{
		m_render_system = std::make_unique<wr::D3D12RenderSystem>();
		
		m_frame_graph = std::make_unique<wr::FrameGraph>();

		m_render_system->Init(std::nullopt);
	}
	
	void Renderer::Update()
	{
	}
	
	void Renderer::Destroy()
	{
		// Make sure the GPU has finished executing the final command list before starting the cleanup
		m_render_system->WaitForAllPreviousWork();
		m_frame_graph->Destroy();
		m_render_system.reset();
	}
}