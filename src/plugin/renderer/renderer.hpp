#pragma once
#include <memory>

namespace wr
{
	class CPUTextures;
	class D3D12RenderSystem;
	class Window;
	class SceneGraph;
}

namespace wmr
{

	class ModelManager;
	class TextureManager;
	class FrameGraphManager;

	class renderer
	{
	public:
		renderer();
		~renderer();
		void Initialize();
		void Update();
		void Render();

	private:
		std::unique_ptr<ModelManager> m_model_manager;
		std::unique_ptr<TextureManager> m_texture_manager;
		std::unique_ptr<FrameGraphManager> m_framegraph_manager;
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		std::unique_ptr<wr::Window> m_window;
		std::shared_ptr<wr::SceneGraph> m_scenegraph;
	};
}
