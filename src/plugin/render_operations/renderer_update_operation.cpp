#include "renderer_update_operation.hpp"

// Wisp plug-in
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Maya API
#include <maya/MViewport2Renderer.h>

namespace wmr
{
	RendererUpdateOperation::RendererUpdateOperation(const MString & name)
		: MHWRender::MUserRenderOperation(name)
		, m_renderer(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer())
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
		// Update the Wisp rendering framework to prepare it for rendering
		m_renderer.Update();

		return MStatus::kSuccess;
	}

	bool RendererUpdateOperation::hasUIDrawables() const
	{
		// Not using any UI drawable
		return false;
	}

	bool RendererUpdateOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
