#include "RendererMain.hpp"

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"

namespace wmr::wri
{
	void RendererMain::Initialize()
	{
		m_render_system = std::make_unique<wr::D3D12RenderSystem>();
		m_frame_graph = std::make_unique<wr::FrameGraph>();

		m_render_system->Init(std::nullopt);
	}

	void RendererMain::Update()
	{
		// TODO: Update the framework logic in here!
	}

	void RendererMain::Resize(unsigned int new_width, unsigned int new_height)
	{
	}

	void RendererMain::Cleanup()
	{
		// Make sure the GPU has finished executing the final command list before starting the cleanup
		m_render_system->WaitForAllPreviousWork();
		m_frame_graph->Destroy();
		m_render_system.reset();
	}
}
