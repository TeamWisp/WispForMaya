#pragma once

#include <maya/MViewport2Renderer.h>

namespace wisp
{
	class QuadRendererOverride : public MHWRender::MQuadRender
	{
	public:
		QuadRendererOverride(const MString& t_name);
		~QuadRendererOverride() final override;

	private:
		MHWRender::MClearOperation& clearOperation() final override;
	};
}