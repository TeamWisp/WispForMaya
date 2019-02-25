#include "renderer_copy_operation.hpp"

// Wisp plug-in
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Maya API
#include <maya/MViewport2Renderer.h>

namespace wmr
{
	RendererCopyOperation::RendererCopyOperation(const MString & name)
		: MHWRender::MUserRenderOperation(name)
		, m_renderer(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer())
	{
	}

	RendererCopyOperation::~RendererCopyOperation()
	{
	}

	const MCameraOverride* RendererCopyOperation::cameraOverride()
	{
		// Not using a camera override
		return nullptr;
	}

	MStatus RendererCopyOperation::execute(const MDrawContext& draw_context)
	{
		// Update the Wisp rendering framework to prepare it for rendering
		m_renderer.Update();

		return MStatus::kSuccess;
	}

	bool RendererCopyOperation::hasUIDrawables() const
	{
		// Not using any UI drawable
		return false;
	}

	bool RendererCopyOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
