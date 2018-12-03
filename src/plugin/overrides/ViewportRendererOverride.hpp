#pragma once
#include <maya/MShaderManager.h>

#include <array>
#include <memory>

namespace wmr
{
	class WispViewportRenderer : public MHWRender::MRenderOverride
	{
	public:
		WispViewportRenderer(const MString& t_name);
		~WispViewportRenderer() final override;

	private:
		void ConfigureRenderOperations();
		void SetDefaultColorTextureState();

		MHWRender::DrawAPI supportedDrawAPIs() const final override;
		MHWRender::MRenderOperation* renderOperation() final override;
		
		MStatus setup(const MString& t_destination) final override;
		MStatus cleanup() final override;
		MString uiName() const final override;

		bool startOperationIterator() final override;
		bool nextRenderOperation() final override;
		bool UpdateTextures(MHWRender::MRenderer* t_renderer, MHWRender::MTextureManager* t_texture_manager);

	private:
		MString m_ui_name;

		// Operations and their names
		std::array<std::unique_ptr<MHWRender::MRenderOperation>, 4> m_render_operations;
		MString m_render_operation_names[3];

		// Texture(s) used for the quad render
		MHWRender::MTextureDescription m_color_texture_desc;
		MHWRender::MTextureAssignment m_color_texture;

		int m_current_render_operation;

		bool m_load_images_from_disk;
	};
}