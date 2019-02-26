#pragma once

// Maya API
#include <maya/MShaderManager.h>
#include <maya/MViewport2Renderer.h>

// Wisp forward declarations
namespace wr
{
	struct CPUTexture;
}

namespace wmr
{
	// Forward declarations
	class Renderer;
	class ScreenRenderOperation;

	class RendererCopyOperation final : public MHWRender::MUserRenderOperation
	{
	public:
		RendererCopyOperation(const MString& name, ScreenRenderOperation& blit_operation);
		~RendererCopyOperation();

	private:
		//! Create an empty texture and its description
		/*! When the texture object is created, its texture resource is set to nullptr. The texture description is set
		 *  to the default 2D texture description values. */
		void SetDefaultTextureState() noexcept;

		//! Release allocated textures
		/*! When this function is called, all textures that were in use by the plug-in will be released and cleaned-up.*/
		void ReleaseTextures() noexcept;

		//! Create a new color texture
		/*! \param cpu_data: Data from the Wisp framework.
		 *  \param created_new_texture: Indicates whether the function created a new texture or updated an existing one. */
		void CreateColorTextureOfSize(const wr::CPUTexture& cpu_data, bool& created_new_texture);

		//! Create a new depth texture
		/*! \param cpu_data: Data from the Wisp framework.
		 *  \param created_new_texture: Indicates whether the function created a new texture or updated an existing one. */
		void CreateDepthTextureOfSize(const wr::CPUTexture& cpu_data, bool& created_new_texture);

		//! Just overriding because we have to, this plug-in does not override the default camera
		const MCameraOverride* cameraOverride() override;
		
		//! Function that allows us to run custom code in this render operation
		MStatus execute(const MDrawContext& draw_context) override;
		
		//! Not using any HUD
		bool hasUIDrawables() const override;

		//! This plug-in does not require any light data
		bool requiresLightData() const override;

	private:
		Renderer& m_renderer;									//!< Wisp renderer
		ScreenRenderOperation& m_blit_operation;				//!< Render operation that blits to the fullscreen quad

		MHWRender::MTextureDescription m_color_texture_desc;	//!< Plug-in color buffer description
		MHWRender::MTextureAssignment m_color_texture;			//!< Plug-in color buffer texture
		MHWRender::MTextureDescription m_depth_texture_desc;	//!< Plug-in depth buffer description
		MHWRender::MTextureAssignment m_depth_texture;			//!< Plug-in depth buffer texture
	};
}