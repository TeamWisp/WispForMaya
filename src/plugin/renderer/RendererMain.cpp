#include "RendererMain.hpp"

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"

namespace wmr::wri
{
	void RendererMain::Initialize()
	{
		// Create a new thread for the renderer to live on
		// This allows Maya to render at its own pace without slowing down the renderer

	}

	void RendererMain::Update()
	{
		// TODO: Update the framework logic in here!
	}

	void RendererMain::Resize(unsigned int new_width, unsigned int new_height)
	{
		// TODO: Resize the renderer!
	}

	void RendererMain::Cleanup()
	{
	}
}
