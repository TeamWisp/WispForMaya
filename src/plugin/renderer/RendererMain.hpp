#pragma once

#include "imgui_tools.hpp"

#include <memory>

namespace wr
{
	class FrameGraph;
	class SceneGraph;
}

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void StartWispRenderer();
		void StopWispRenderer();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		std::unique_ptr<wr::FrameGraph> m_frame_graph;
		std::unique_ptr<wr::SceneGraph> m_scene_graph;
	};
}