#include "renderer_draw_operation.hpp"

// Wisp plug-in
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Maya API
#include <maya/MViewport2Renderer.h>

namespace wmr
{
	RendererDrawOperation::RendererDrawOperation(const MString & name)
		: MHWRender::MUserRenderOperation(name)
		, m_renderer(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer())
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
		// Render the scene using the Wisp rendering framework
		m_renderer.Render();

		return MStatus::kSuccess;
	}

	bool RendererDrawOperation::hasUIDrawables() const
	{
		// Not using any UI drawable
		return false;
	}

	bool RendererDrawOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
