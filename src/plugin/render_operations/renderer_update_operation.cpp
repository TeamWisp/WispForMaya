#include "renderer_update_operation.hpp"

namespace wmr
{
	RendererUpdateOperation::RendererUpdateOperation(const MString & name)
		: MHWRender::MUserRenderOperation(name)
	{
	}

	RendererUpdateOperation::~RendererUpdateOperation()
	{
	}

	const MCameraOverride* RendererUpdateOperation::cameraOverride()
	{
		// Not using a camera override
		return nullptr;
	}

	MStatus RendererUpdateOperation::execute(const MDrawContext& draw_context)
	{
		return MStatus::kSuccess;
	}

	bool RendererUpdateOperation::hasUIDrawables() const
	{
		// Not using any UI drawables
		return false;
	}

	bool RendererUpdateOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
