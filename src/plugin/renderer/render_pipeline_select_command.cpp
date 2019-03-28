#include "render_pipeline_select_command.hpp"

// Wisp plug-in
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"

// Maya API
#include <maya/MArgList.h>
#include <maya/MViewport2Renderer.h>

MStatus wmr::RenderPipelineSelectCommand::doIt(const MArgList& args)
{
	// Get the frame graph in the renderer
	FrameGraphManager& frame_graph = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetFrameGraph();

	// Invalid number of arguments
	if (args.length() != 1)
		return MStatus::kFailure;

	// Arguments:
	//  0 -> deferred
	//  1 -> hybrid ray-tracing
	//  2 -> full ray-tracing
	auto arg = args.asInt(0);

	switch (arg)
	{
		// Deferred
		case 0:
			frame_graph.SetType(RendererFrameGraphType::DEFERRED);
			break;

		// Hybrid ray-tracing
		case 1:
			frame_graph.SetType(RendererFrameGraphType::HYBRID_RAY_TRACING);
			break;

		// Full ray-tracing
		case 2:
			frame_graph.SetType(RendererFrameGraphType::FULL_RAY_TRACING);
			break;

		// Invalid argument
		default:
			break;
	}

	return MStatus::kSuccess;
}
