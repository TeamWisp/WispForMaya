// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "render_pipeline_select_command.hpp"

// Wisp
#include <d3d12/d3d12_renderer.hpp>
#include <util/log.hpp>

// Wisp plug-in
#include "miscellaneous/maya_popup.hpp"
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
		static bool first_time_hybrid = true;
		if (first_time_hybrid && !renderer.GetD3D12Renderer().m_device->m_dxr_support)
		{
			first_time_hybrid = false;

			// Open warning popup
			MayaPopup::Options options;
			options.window_name = "hybrid_open_popup";
			options.width = 500;

			MayaPopup::SpawnFromFile("resources/hybrid_switch.txt", options);
		}
		else 
		{
			frame_graph.SetType(RendererFrameGraphType::HYBRID_RAY_TRACING);
		}
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
