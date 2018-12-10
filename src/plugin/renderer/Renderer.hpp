#pragma once

#include <memory>

namespace wr
{
	class D3D12RenderSystem;
	class FrameGraph;
	class SceneGraph;
}

namespace wmr::wri
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Initialize(unsigned int render_target_width, unsigned int render_target_height);
		void Update();
		void Destroy();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		std::unique_ptr<wr::FrameGraph> m_frame_graph;
		std::shared_ptr<wr::SceneGraph> m_scene_graph;
	};
}
