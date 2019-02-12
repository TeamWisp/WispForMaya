#include "UIOverride.hpp"

namespace wmr
{
	WispUIRenderer::WispUIRenderer(const MString & name)
		: MHWRender::MSceneRender(name)
	{
	}

	WispUIRenderer::~WispUIRenderer()
	{
	}

	MHWRender::MSceneRender::MSceneFilterOption WispUIRenderer::renderFilterOverride()
	{
		return MHWRender::MSceneRender::kRenderNonShadedItems;
	}

	MHWRender::MClearOperation& WispUIRenderer::clearOperation()
	{
		// Do not clear anything
		mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearNone));
		return mClearOperation;
	}

	MUint64 WispUIRenderer::getObjectTypeExclusions()
	{
		// Render all UI
		return 0;
	}
}
