#pragma once

#include "scene_graph/camera_node.hpp"

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

		void UpdateCamera();

	private:
		// Camera that mimics the Maya viewport camera
		std::shared_ptr<wr::CameraNode> m_viewport_camera;

		//std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		//std::unique_ptr<wr::FrameGraph> m_frame_graph;
		//std::shared_ptr<wr::SceneGraph> m_scene_graph;
	};
}
