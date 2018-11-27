#include "UIOverride.hpp"

wmr::WispUIRenderer::WispUIRenderer(const MString & t_name)
	: MHWRender::MSceneRender(t_name)
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
	// Exclude the grid and image planes from the final render
	return (MHWRender::MFrameContext::kExcludeGrid | MHWRender::MFrameContext::kExcludeImagePlane);
}
