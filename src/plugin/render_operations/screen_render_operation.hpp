#pragma once

// Maya API
#include <maya/MShaderManager.h>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Full screen quad renderer override implementation
	/*! Implementation of a Maya MQuadRender. It inherits from the Maya API base class and implements all methods needed
	 *  to make the override work. The code style for functions is a bit different here because our style guide differs
	 *  from the style used for the Maya API. */
	class ScreenRenderOperation final : public MHWRender::MQuadRender
	{
	public:
		//! Sets all variables to their default values
		ScreenRenderOperation(const MString& name);

		//! Clean-up after the plug-in
		/*! Releases the resources used in the plug-in. Loaded shaders will be unloaded and cleaned up, render and depth 
		 *  textures are deallocated.*/
		~ScreenRenderOperation() override;

		//! Set the active color texture
		/*! Sets the color texture resource to whatever texture is passed as an argument. Triggers the
		 *  m_color_texture_changed flag.
		 *  
		 *  /param color_texture The new color texture to use for the fullscreen quad renderer.
		 *  /sa m_color_texture_changed */
		void SetColorTexture(const MHWRender::MTextureAssignment& color_texture);

		//! Set the active depth texture
		/*! Sets the depth texture resource to whatever texture is passed as an argument. Triggers the
		 *  m_depth_texture_changed flag.
		 *
		 *  /param color_texture The new depth texture to use for the fullscreen quad renderer.
		 *  /sa m_depth_texture_changed */
		void SetDepthTexture(const MHWRender::MTextureAssignment& depth_texture);

	private:
		//! Retrieve a hardware shader and configure it
		/*! Tries to get a hold of a hardware shader and configures the correct parameters. Please note that this function
		 *  is an override of a Maya API function, so check the Autodesk documentation for more information.
		 *  
		 *  /return Returns the current shader instance stored in the class, nullptr is returned when the shader member is unassigned. */
		const MHWRender::MShaderInstance* shader() override;

		//! Sets the depth stencil buffer description
		/*! Please note that this is a function override from the Maya API, so check the Autodesk documentation for more
		 *  information.
		 *  
		 *  /return Returns the depth stencil buffer description. */
		const MHWRender::MDepthStencilState* depthStencilStateOverride() override;

		//! Configure the clear operation for the fullscreen quad renderer
		/*! Please note that this is a function override from the Maya API, so check the Autodesk documentation for more
		 *  information.
		 *  
		 *  /return Returns the clear operation data structure as seen in the Maya API. */
		MHWRender::MClearOperation& clearOperation() override;

	private:
		// Hardware shader used to render the fullscreen quad
		MHWRender::MShaderInstance* m_shader_instance;

		//! Color texture to use while rendering
		/*! This texture is not managed by this class, which is why there is no method to release it. */
		MHWRender::MTextureAssignment m_color_texture;

		//! Depth texture to use while rendering
		/*! This texture is not managed by this class, which is why there is no method to release it. */
		MHWRender::MTextureAssignment m_depth_texture;

		//! Description of the currently active depth buffer
		const MHWRender::MDepthStencilState* m_depth_stencil_state;

		//! Set when assigning a new color texture, this makes the renderer use the new texture during the next frame.
		bool m_color_texture_changed;

		//! Set when assigning a new depth texture, this makes the renderer use the new texture during the next frame.
		bool m_depth_texture_changed;
	};
}