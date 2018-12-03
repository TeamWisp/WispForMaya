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

		void Initialize();

	private:
		// Set the names of the render operations
		void ConfigureRenderOperations();

		void SetDefaultColorTextureState();
		void ReleaseColorTextureResources() const;
		void CreateRenderOperations();

		// Which Maya rendering back ends are supported by this plug-in?
		MHWRender::DrawAPI supportedDrawAPIs() const final override;

		// Loop through all render operations and return the current active operation to Maya
		MHWRender::MRenderOperation* renderOperation() final override;
		
		// Called when the viewport needs to be refreshed (no updates if nothing changes)
		MStatus setup(const MString& t_destination) final override;

		bool AreAllRenderOperationsSetCorrectly() const;

		// Update the Maya color texture
		bool UpdateTextures(MHWRender::MRenderer* t_renderer, MHWRender::MTextureManager* t_texture_manager);

		void EnsurePanelDisplayShading(const MString& t_destination);

		MStatus cleanup() final override;

		// Returns the name of the plug-in that should show up under the "renderer" drop-down menu in the Maya viewport
		MString uiName() const final override;

		bool startOperationIterator() final override;
		bool nextRenderOperation() final override;

	private:
		MString m_ui_name;

		std::array<std::unique_ptr<MHWRender::MRenderOperation>, 4> m_render_operations;
		MString m_render_operation_names[3];

		MHWRender::MTextureDescription m_color_texture_desc;
		MHWRender::MTextureAssignment m_color_texture;

		int m_current_render_operation;

		bool m_load_images_from_disk;
	};
}