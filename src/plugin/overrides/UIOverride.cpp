#include "UIOverride.hpp"

wmr::WispUIRenderer::WispUIRenderer(const MString & name)
	: MHWRender::MSceneRender(name)
{
}

wmr::WispUIRenderer::~WispUIRenderer()
{
}

MHWRender::MSceneRender::MSceneFilterOption wmr::WispUIRenderer::renderFilterOverride()
{
	return MHWRender::MSceneRender::kRenderNonShadedItems;
}

MHWRender::MClearOperation& wmr::WispUIRenderer::clearOperation()
{
	mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearNone));
	return mClearOperation;
}

MUint64 wmr::WispUIRenderer::getObjectTypeExclusions()
{
	// Render all UI
	return 0;
}
