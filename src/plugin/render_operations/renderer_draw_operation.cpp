#include "renderer_draw_operation.hpp"

namespace wmr
{
	RendererDrawOperation::RendererDrawOperation(const MString & name)
		: MHWRender::MUserRenderOperation(name)
	{
	}

	RendererDrawOperation::~RendererDrawOperation()
	{
	}

	const MCameraOverride* RendererDrawOperation::cameraOverride()
	{
		// Not using a camera override
		return nullptr;
	}

	MStatus RendererDrawOperation::execute(const MDrawContext& draw_context)
	{
		return MStatus::kSuccess;
	}

	bool RendererDrawOperation::hasUIDrawables() const
	{
		// Not using any UI drawables
		return false;
	}

	bool RendererDrawOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
