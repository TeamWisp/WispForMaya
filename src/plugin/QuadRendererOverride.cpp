#include "QuadRendererOverride.hpp"

wisp::QuadRendererOverride::QuadRendererOverride(const MString& t_name)
	: MQuadRender(t_name)
{
}

wisp::QuadRendererOverride::~QuadRendererOverride()
{
}

MHWRender::MClearOperation& wisp::QuadRendererOverride::clearOperation()
{
	float clearColorTest[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	mClearOperation.setClearColor(clearColorTest);
	mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearAll));

	return mClearOperation;
}
