#include "gizmo_render_operation.hpp"

namespace wmr
{
	GizmoRenderOperation::GizmoRenderOperation(const MString & name)
		: MHWRender::MSceneRender(name)
	{
	}

	MHWRender::MSceneRender::MSceneFilterOption GizmoRenderOperation::renderFilterOverride()
	{
		return MHWRender::MSceneRender::kRenderNonShadedItems;
	}

	MHWRender::MClearOperation& GizmoRenderOperation::clearOperation()
	{
		// Do not clear anything
		mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearNone));
		return mClearOperation;
	}

	MUint64 GizmoRenderOperation::getObjectTypeExclusions()
	{
		// Render all UI
		return MObjectTypeExclusions::kExcludeNone;
	}
}
