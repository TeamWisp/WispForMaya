#pragma once
#include <maya/MShaderManager.h>

namespace wmr
{
	class WispScreenBlitter : public MHWRender::MQuadRender
	{
	public:
		WispScreenBlitter(const MString& name);
		~WispScreenBlitter() final override;

		void SetColorTexture(const MHWRender::MTextureAssignment& color_texture);
		void SetDepthTexture(const MHWRender::MTextureAssignment& depth_texture);

	private:
		const MHWRender::MShaderInstance* shader() final override;
		const MHWRender::MDepthStencilState* depthStencilStateOverride() final override;
		MHWRender::MClearOperation& clearOperation() final override;

	private:
		// TODO: Write a shader to render a colored texture to a fullscreen quad
		MHWRender::MShaderInstance* m_shader_instance;

		// These textures are not managed by this class, so there are no methods to release the textures
		MHWRender::MTextureAssignment m_color_texture;
		MHWRender::MTextureAssignment m_depth_texture;

		const MHWRender::MDepthStencilState* m_depth_stencil_state;

		bool m_color_texture_changed;
		bool m_depth_texture_changed;
	};
}