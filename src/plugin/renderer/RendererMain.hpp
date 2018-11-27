#pragma once

#include "imgui_tools.hpp"

#include <memory>

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void StartWispRenderer();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		
	};
}