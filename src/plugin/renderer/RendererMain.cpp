#include "RendererMain.hpp"

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"

void wmr::wri::RendererMain::Initialize()
{
	m_render_system = std::make_unique<wr::D3D12RenderSystem>();
	m_frame_graph = std::make_unique<wr::FrameGraph>();

	m_render_system->Init(std::nullopt);
}

void wmr::wri::RendererMain::Update()
{
	// TODO: Update the framework logic in here!
}

void wmr::wri::RendererMain::Resize(unsigned int t_new_width, unsigned int t_new_height)
{
}

void wmr::wri::RendererMain::Cleanup()
{
	// Make sure the GPU has finished executing the final command list before starting the cleanup
	m_render_system->WaitForAllPreviousWork();
	m_frame_graph->Destroy();
	m_render_system.reset();
}
