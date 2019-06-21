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

// Wisp
#include <d3d12/d3d12_renderer.hpp>
#include <util/log.hpp>
#include <scene_graph/camera_node.hpp>

// Wisp plug-in
#include "render_pipeline_select_command.hpp"
#include "miscellaneous/maya_popup.hpp"
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/renderer.hpp"

// Maya API
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MSyntax.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MGlobal.h>

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

	// Camera so we can set depth of field data
	auto camera = renderer.GetScenegraph().GetActiveCamera();

	if (arg_data.isFlagSet(PIPELINE_SHORT_FLAG))
	{
		auto param_0 = arg_data.flagArgumentInt(PIPELINE_SHORT_FLAG, 0);

		if (param_0 == 0)
		{
			frame_graph.SetType(RendererFrameGraphType::DEFERRED);
			return MStatus::kSuccess;
		}
		else if (param_0 == 1)
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

			return MStatus::kSuccess;
		}
	}
	else if (arg_data.isFlagSet(SKYBOX_SHORT_FLAG))
	{
		auto param_0 = arg_data.flagArgumentString(SKYBOX_SHORT_FLAG, 0);

		try
		{
			renderer.UpdateSkybox(param_0.asChar());
		}
		catch (std::exception& e)
		{
			LOGE("Could not load skybox texture {}, probably invalid file extension. {}", param_0.asChar(), e.what());
			return MStatus::kFailure;
		}

		return MStatus::kSuccess;
	}
	else if (arg_data.isFlagSet(DOF_ENABLE_SHORT_FLAG))
	{
		camera->m_enable_dof = arg_data.flagArgumentBool(DOF_ENABLE_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_FNUM_SHORT_FLAG))
	{
		camera->m_f_number = arg_data.flagArgumentDouble(DOF_FNUM_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_FOCAL_LENGTH_SHORT_FLAG))
	{
		camera->m_film_size = arg_data.flagArgumentDouble(DOF_FOCAL_LENGTH_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG))
	{
		camera->m_shape_amt = arg_data.flagArgumentDouble(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_FOCAL_LENGTH_SHORT_FLAG))
	{
		camera->m_focal_length = arg_data.flagArgumentDouble(DOF_FOCAL_LENGTH_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_FOCAL_PLANE_DISTANCE_SHORT_FLAG))
	{
		camera->m_focus_dist = arg_data.flagArgumentDouble(DOF_FOCAL_PLANE_DISTANCE_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG))
	{
		camera->m_aperture_blades = arg_data.flagArgumentInt(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG, 0);
	}

	// Auto-focus enabled
	if (arg_data.flagArgumentBool(DOF_AUTO_FOCUS_SHORT_FLAG, 1))
	{
		camera->m_focus_dist = 0.0f;
	}

	return MStatus::kSuccess;
}

MSyntax wmr::RenderPipelineSelectCommand::create_syntax()
{
	MSyntax syntax;

	syntax.addFlag(PIPELINE_SHORT_FLAG, PIPELINE_LONG_FLAG, MSyntax::kUnsigned);
	syntax.addFlag(SKYBOX_SHORT_FLAG, SKYBOX_LONG_FLAG, MSyntax::kString);
	syntax.addFlag(DOF_ENABLE_SHORT_FLAG, DOF_ENABLE_LONG_FLAG, MSyntax::kBoolean);
	syntax.addFlag(DOF_AUTO_FOCUS_SHORT_FLAG, DOF_AUTO_FOCUS_LONG_FLAG, MSyntax::kBoolean);
	syntax.addFlag(DOF_FNUM_SHORT_FLAG, DOF_FNUM_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_FILM_SIZE_SHORT_FLAG, DOF_FILM_SIZE_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG, DOF_BOKEH_SHAPE_SIZE_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_FOCAL_LENGTH_SHORT_FLAG, DOF_FOCAL_LENGTH_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_FOCAL_PLANE_DISTANCE_SHORT_FLAG, DOF_FOCAL_PLANE_DISTANCE_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG, DOF_APERTURE_BLADE_COUNT_LONG_FLAG, MSyntax::kUnsigned);

	syntax.enableQuery(true);
	syntax.enableEdit(false);

	LOG("Finished creating custom MEL command syntax.");

	return syntax;
}
