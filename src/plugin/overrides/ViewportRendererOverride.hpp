#pragma once
#include <maya/MShaderManager.h>

#include <memory>

namespace wmr
{
	class WispViewportRenderer : public MHWRender::MRenderOverride
	{
	public:
		WispViewportRenderer(const MString& t_name);
		~WispViewportRenderer() override;

	private:
		// ============================================================

		MHWRender::DrawAPI supportedDrawAPIs() const override;
		MHWRender::MRenderOperation* renderOperation() override;
		
		MStatus setup(const MString& t_destination) override;
		MStatus cleanup() override;
		MString uiName() const override;

		bool startOperationIterator() override;
		bool nextRenderOperation() override;

		// ============================================================

		bool UpdateTextures(MHWRender::MRenderer* t_renderer, MHWRender::MTextureManager* t_texture_manager);

		// ============================================================

	private:
		MString m_ui_name;

		// Operations and their names
		MHWRender::MRenderOperation* m_render_operations[4];
		MString m_render_operation_names[3];

		// Texture(s) used for the quad render
		MHWRender::MTextureDescription m_color_texture_desc;
		MHWRender::MTextureAssignment m_color_texture;

		int m_current_render_operation;

		bool m_load_images_from_disk;
	};
}