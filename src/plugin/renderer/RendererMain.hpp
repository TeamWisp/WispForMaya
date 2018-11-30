#pragma once

#include <memory>

namespace wr
{
	class Texture;
	class D3D12RenderSystem;
	class SceneGraph;
	class FrameGraph;
}

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void StartWispRenderer();

		// The result is the texture data for this frame
		wr::Texture* UpdateWispRenderer();
		void StopWispRenderer();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> t_render_system;
		std::shared_ptr<wr::SceneGraph> t_scene_graph;
		std::unique_ptr<wr::FrameGraph> t_frame_graph;
	};
}