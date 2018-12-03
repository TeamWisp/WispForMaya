#pragma once
#include <maya/MShaderManager.h>

namespace wmr
{
	class WispScreenBlitter : public MHWRender::MQuadRender
	{
	public:
		WispScreenBlitter(const MString& t_name);
		~WispScreenBlitter() final override;

		void SetColorTexture(const MHWRender::MTextureAssignment& t_color_texture);

	private:
		const MHWRender::MShaderInstance* shader() final override;
		MHWRender::MClearOperation& clearOperation() final override;

	private:
		// TODO: Write a shader to render a colored texture to a fullscreen quad
		MHWRender::MShaderInstance* m_shader_instance;

		// This texture is not managed by this class, so there are no methods to release the textures
		MHWRender::MTextureAssignment m_color_texture;

		bool m_color_texture_changed;
	};
}