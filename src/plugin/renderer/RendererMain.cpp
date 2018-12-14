#include "RendererMain.hpp"

#include "Renderer.hpp"

namespace wmr::wri
{
	RendererMain::RendererMain()
	{
	}

	RendererMain::~RendererMain()
	{
	}

	void RendererMain::Initialize()
	{
		m_renderer->Initialize(1280, 720);
	}

	void RendererMain::Update()
	{
		m_renderer->Update();
	}

	void RendererMain::Resize(unsigned int new_width, unsigned int new_height)
	{
		// TODO: Resize the renderer!
	}

	void RendererMain::Cleanup()
	{
		m_renderer->Destroy();
	}
}
