#include "render_pipeline_select_command.hpp"

// Wisp
#include <util/log.hpp>

// Wisp plug-in
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"

// Maya API
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>

// C++ standard
#include <string>

MStatus wmr::RenderPipelineSelectCommand::doIt(const MArgList& args)
{
	MStatus status;

	MArgParser arg_data(syntax(), args, &status);

	// Get the frame graph in the renderer
	auto viewport_override = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME));

	// The viewport override has not been initialized properly yet
	if (!viewport_override->IsInitialized())
	{
		LOGE("Viewport override has not been initialized properly yet.");
		return MStatus::kFailure;
	}

	auto& renderer = viewport_override->GetRenderer();
	auto& frame_graph = renderer.GetFrameGraph();

	bool deferred_set = arg_data.isFlagSet("deferred");
	bool hybrid_set = arg_data.isFlagSet("hybrid_ray_trace");

	auto skybox_path = arg_data.commandArgumentString(0);

	if (deferred_set)
	{
		frame_graph.SetType(RendererFrameGraphType::DEFERRED);
	}
	else if (hybrid_set)
	{
		frame_graph.SetType(RendererFrameGraphType::HYBRID_RAY_TRACING);
	}
	else
	{
		try
		{
			renderer.UpdateSkybox(skybox_path.asChar());
		}
		catch (std::exception& e)
		{
			LOGE("Could not load skybox texture {}, probably invalid file extension. {}", skybox_path.asChar(), e.what());
			return MStatus::kFailure;
		}

		return MStatus::kSuccess;
	}

	return MStatus::kSuccess;
}

MSyntax wmr::RenderPipelineSelectCommand::create_syntax()
{
	MSyntax syntax;

	syntax.addArg(MSyntax::kString);
	syntax.addFlag("-d", "-deferred",			MSyntax::kNoArg);
	syntax.addFlag("-h", "-hybrid_ray_trace",	MSyntax::kNoArg);

	syntax.enableQuery(false);
	syntax.enableEdit(false);

	LOG("Finished creating custom MEL command syntax.");

	return syntax;
}
