#pragma once
#include <memory>

#include "frame_graph/frame_graph.hpp"

namespace wr
{
	class D3D12RenderSystem;
	class Window;
	class SceneGraph;
	class CameraNode;
}

namespace wmr
{

	class ModelManager;
	class MaterialManager;
	class TextureManager;
	class FrameGraphManager;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void Initialize() noexcept;
		void Update();
		void Render();
		const wr::CPUTextures GetRenderResult();

		ModelManager& GetModelManager() const;
		FrameGraphManager& GetFrameGraph() const;
		MaterialManager& GetMaterialManager() const;
		TextureManager& GetTextureManager() const;
		wr::SceneGraph& GetScenegraph() const;
		wr::D3D12RenderSystem& GetD3D12Renderer() const;
		std::shared_ptr<wr::CameraNode> GetCamera() const;


	private:
		std::unique_ptr<FrameGraphManager>		m_framegraph_manager;
		std::unique_ptr<MaterialManager>		m_material_manager;
		std::unique_ptr<ModelManager>			m_model_manager;
		std::unique_ptr<TextureManager>			m_texture_manager;

		std::unique_ptr<wr::D3D12RenderSystem>	m_render_system;
		std::shared_ptr<wr::SceneGraph>			m_scenegraph;
		std::unique_ptr<wr::Window>				m_window;
		std::shared_ptr<wr::CameraNode>			m_wisp_camera;

		wr::CPUTextures							m_result_textures;

	};
}
