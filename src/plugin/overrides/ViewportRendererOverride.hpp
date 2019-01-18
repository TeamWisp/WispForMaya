#pragma once
#include <maya/MShaderManager.h>

#include <array>
#include <memory>

namespace wmr::wri
{
	class Renderer;
}

namespace wr
{
	struct CameraNode;
	struct CPUTexture;
	struct CPUTextures;

	class AssimpModelLoader;
	class D3D12RenderSystem;
	class FrameGraph;
	class SceneGraph;
	class TexturePool;
}

namespace wmr
{
	class ScenegraphParser;

	enum class WispBufferType
	{
		COLOR,
		DEPTH
	};

	class ViewportRenderer final : public MHWRender::MRenderOverride
	{
	public:
		ViewportRenderer(const MString& name);
		~ViewportRenderer() override;

		void Initialize();
		void Destroy();

	private:
		// Set the names of the render operations
		void ConfigureRenderOperations();

		void SetDefaultTextureState();
		void ReleaseTextureResources() const;
		void CreateRenderOperations();
		void InitializeWispRenderer();

		// Which Maya rendering back ends are supported by this plug-in?
		MHWRender::DrawAPI supportedDrawAPIs() const override;

		// Loop through all render operations and return the current active operation to Maya
		MHWRender::MRenderOperation* renderOperation() override;
		
		// Called when the viewport needs to be refreshed (no updates if nothing changes)
		MStatus setup(const MString& destination) override;

		bool AreAllRenderOperationsSetCorrectly() const;

		// Update the Maya color texture
		bool UpdateTextures(MHWRender::MRenderer* maya_renderer, MHWRender::MTextureManager* texture_manager, const wr::CPUTextures& cpu_textures);

		// Update the texture pixel data
		void UpdateTextureData(MHWRender::MTextureAssignment& texture_to_update, WispBufferType type, const wr::CPUTexture& cpu_texture, MHWRender::MTextureManager* texture_manager);

		MStatus cleanup() override;

		// Returns the name of the plug-in that should show up under the "renderer" drop-down menu in the Maya viewport
		MString uiName() const override;

		bool startOperationIterator() override;
		bool nextRenderOperation() override;

	private:
		void SynchronizeWispWithMayaViewportCamera();


		MString m_ui_name;

		// Render operations
		std::array<std::unique_ptr<MHWRender::MRenderOperation>, 4> m_render_operations;
		MString m_render_operation_names[3];
		int m_current_render_operation;

		// Wisp render output color
		MHWRender::MTextureDescription m_color_texture_desc;
		MHWRender::MTextureAssignment m_color_texture;

		// Wisp render output depth
		MHWRender::MTextureDescription m_depth_texture_desc;
		MHWRender::MTextureAssignment m_depth_texture;

		// Render system 
		std::unique_ptr<wr::AssimpModelLoader> m_model_loader;
		std::shared_ptr<wr::CameraNode> m_viewport_camera;
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		std::unique_ptr<wr::FrameGraph> m_framegraph;
		std::shared_ptr<wr::SceneGraph> m_scenegraph;
		std::shared_ptr<wr::TexturePool> m_texture_pool;

		std::unique_ptr<wmr::ScenegraphParser> m_scenegraph_parser;
	};
}