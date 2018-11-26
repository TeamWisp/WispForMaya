#include "UIOverride.hpp"

wisp::UIOverride::UIOverride(const MString& t_name)
	: MHWRender::MSceneRender(t_name)
{
}

wisp::UIOverride::~UIOverride()
{
}

MHWRender::MSceneRender::MSceneFilterOption wisp::UIOverride::renderFilterOverride()
{
	// TODO: Figure out what this does
	return MHWRender::MSceneRender::kRenderNonShadedItems;
}

MUint64 wisp::UIOverride::getObjectTypeExclusions()
{
	// TODO: Set proper flags
	return 0;
}

MHWRender::MClearOperation& wisp::UIOverride::clearOperation()
{
	mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearNone));
	return mClearOperation;
}
