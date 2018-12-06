#pragma once

#include "frame_graph/frame_graph.hpp"

#include <memory>

namespace wr
{
	class D3D12RenderSystem;
	class SceneGraph;
}

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain() = default;
		~RendererMain() = default;

		void Initialize();
		void Update();
		void Resize(unsigned int new_width, unsigned int new_height);
		void Cleanup();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		std::unique_ptr<wr::FrameGraph> m_frame_graph;
		std::shared_ptr<wr::SceneGraph> m_scene_graph;
	};
}