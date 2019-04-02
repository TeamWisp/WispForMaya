#include "render_pipeline_select_command.hpp"

// Wisp plug-in
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"

// Maya API
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>

MStatus wmr::RenderPipelineSelectCommand::doIt(const MArgList& args)
{
	MStatus status;

	MArgParser arg_data(syntax(), args, &status);

	// Get the frame graph in the renderer
	auto viewport_override = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME));

	// The viewport override has not been initialized properly yet
	if (!viewport_override->IsInitialized())
		return MStatus::kFailure;

	auto& renderer = viewport_override->GetRenderer();
	auto& frame_graph = renderer.GetFrameGraph();

	bool deferred_set = arg_data.isFlagSet("deferred");
	bool hybrid_set = arg_data.isFlagSet("hybrid_ray_trace");
	bool full_rt_set = arg_data.isFlagSet("full_ray_trace");
	bool path_trace_set = arg_data.isFlagSet("path_trace");

	if (deferred_set)
	{
		frame_graph.SetType(RendererFrameGraphType::DEFERRED);
	}
	else if (hybrid_set)
	{
		frame_graph.SetType(RendererFrameGraphType::HYBRID_RAY_TRACING);
	}
	else if (full_rt_set)
	{
		frame_graph.SetType(RendererFrameGraphType::FULL_RAY_TRACING);
	}
	else if (path_trace_set)
	{
		frame_graph.SetType(RendererFrameGraphType::PATH_TRACER);
	}
	else
	{
		// Invalid
		return MStatus::kFailure;
	}

	return MStatus::kSuccess;
}

MSyntax wmr::RenderPipelineSelectCommand::create_syntax()
{
	MSyntax syntax;

	syntax.addFlag("-d", "-deferred",			MSyntax::kNoArg);
	syntax.addFlag("-h", "-hybrid_ray_trace",	MSyntax::kNoArg);
	syntax.addFlag("-f", "-full_ray_trace",		MSyntax::kNoArg);
	syntax.addFlag("-p", "-path_trace",			MSyntax::kNoArg);

	syntax.enableQuery(false);
	syntax.enableEdit(false);

	return syntax;
}
