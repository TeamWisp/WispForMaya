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
#include <scene_graph/camera_node.hpp>
#include <util/log.hpp>

// Wisp plug-in
#include "miscellaneous/maya_popup.hpp"
#include "miscellaneous/render_settings.hpp"
#include "plugin/framegraph/frame_graph_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/renderer.hpp"
#include "render_pipeline_select_command.hpp"

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

	// Camera so we can set depth of field data
	auto camera = renderer.GetScenegraph().GetActiveCamera();

	// Bloom settings for all frame graphs
	auto bloom_settings_deferred = GetRenderSettings<wr::BloomSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::DEFERRED));
	auto bloom_settings_hybrid = GetRenderSettings<wr::BloomSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING));

	// NVIDIA HBAO settings
	auto hbao_settings = GetRenderSettings<wr::HBAOSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::DEFERRED));

	// RTX AO settings
	auto rtao_settings = GetRenderSettings<wr::RTAOSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING));

	// Acceleration structure settings
	auto as_build_settings = GetRenderSettings<wr::ASBuildSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING));

	// RT shadow settings
	auto rt_shadow_settings = GetRenderSettings<wr::RTShadowSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING));
	auto rt_shadow_denoiser_settings = GetRenderSettings<wr::ShadowDenoiserSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING));

	//#TODO: Refactor this, it is a real mess...
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
	else if (arg_data.isFlagSet(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG))
	{
		camera->m_shape_amt = arg_data.flagArgumentDouble(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG))
	{
		camera->m_aperture_blades = arg_data.flagArgumentInt(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(BLOOM_ENABLE_SHORT_FLAG))
	{
		bool state = arg_data.flagArgumentBool(BLOOM_ENABLE_SHORT_FLAG, 0);
		bloom_settings_deferred->m_runtime.m_enable_bloom = state;
		bloom_settings_hybrid->m_runtime.m_enable_bloom = state;
	}
	else if (arg_data.isFlagSet(HBAO_METERS_TO_UNITS_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_meters_to_view_space_units = arg_data.flagArgumentDouble(HBAO_METERS_TO_UNITS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(HBAO_RADIUS_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_radius = arg_data.flagArgumentDouble(HBAO_RADIUS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(HBAO_BIAS_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_bias = arg_data.flagArgumentDouble(HBAO_BIAS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(HBAO_POWER_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_power_exp = arg_data.flagArgumentDouble(HBAO_POWER_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(HBAO_BLUR_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_enable_blur = arg_data.flagArgumentBool(HBAO_BLUR_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(HBAO_BLUR_SHARPNESS_SHORT_FLAG))
	{
		hbao_settings->m_runtime.m_blur_sharpness = arg_data.flagArgumentDouble(HBAO_BLUR_SHARPNESS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RTAO_BIAS_SHORT_FLAG))
	{
		rtao_settings->m_runtime.bias = arg_data.flagArgumentDouble(RTAO_BIAS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RTAO_RADIUS_SHORT_FLAG))
	{
		rtao_settings->m_runtime.radius = arg_data.flagArgumentDouble(RTAO_RADIUS_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RTAO_POWER_SHORT_FLAG))
	{
		rtao_settings->m_runtime.power = arg_data.flagArgumentDouble(RTAO_POWER_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RTAO_SAMPLES_PER_PIXEL_SHORT_FLAG))
	{
		rtao_settings->m_runtime.sample_count = arg_data.flagArgumentDouble(RTAO_SAMPLES_PER_PIXEL_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_EPSILON_SHORT_FLAG))
	{
		rt_shadow_settings->m_runtime.m_epsilon = arg_data.flagArgumentDouble(RT_SHADOWS_EPSILON_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_SAMPLES_PER_PIXEL_SHORT_FLAG))
	{
		rt_shadow_settings->m_runtime.m_sample_count = arg_data.flagArgumentInt(RT_SHADOWS_SAMPLES_PER_PIXEL_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_DENOISER_ALPHA_SHORT_FLAG))
	{
		rt_shadow_denoiser_settings->m_runtime.m_alpha = arg_data.flagArgumentDouble(RT_SHADOWS_DENOISER_ALPHA_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_DENOISER_MOMENTS_ALPHA_SHORT_FLAG))
	{
		rt_shadow_denoiser_settings->m_runtime.m_moments_alpha = arg_data.flagArgumentDouble(RT_SHADOWS_DENOISER_MOMENTS_ALPHA_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG))
	{
		rt_shadow_denoiser_settings->m_runtime.m_l_phi = arg_data.flagArgumentDouble(RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_DENOISER_N_PHI_SHORT_FLAG))
	{
		rt_shadow_denoiser_settings->m_runtime.m_n_phi = arg_data.flagArgumentDouble(RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG, 0);
	}
	else if (arg_data.isFlagSet(RT_SHADOWS_DENOISER_Z_PHI_SHORT_FLAG))
	{
		rt_shadow_denoiser_settings->m_runtime.m_z_phi = arg_data.flagArgumentDouble(RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG, 0);
	}

	// Save bloom settings
	if (bloom_settings_deferred.has_value())
	{
		SetRenderSettings<wr::BloomSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::DEFERRED), bloom_settings_deferred.value());
	}
	
	if (bloom_settings_hybrid.has_value())
	{
		SetRenderSettings<wr::BloomSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING), bloom_settings_hybrid.value());
	}

	// Save AO settings
	if (hbao_settings.has_value())
	{
		SetRenderSettings<wr::HBAOSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::DEFERRED), hbao_settings.value());
	}
	
	if (rtao_settings.has_value())
	{
		SetRenderSettings<wr::RTAOSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING), rtao_settings.value());
	}

	// Save acceleration structure settings
	if (as_build_settings.has_value())
	{
		SetRenderSettings<wr::ASBuildSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING), as_build_settings.value());
	}

	// Save shadow settings
	if (rt_shadow_settings.has_value())
	{
		SetRenderSettings<wr::RTShadowSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING), rt_shadow_settings.value());
	}

	// Save shadow denoiser settings
	if (rt_shadow_denoiser_settings.has_value())
	{
		SetRenderSettings<wr::ShadowDenoiserSettings>(frame_graph.GetSpecifiedFramegraph(RendererFrameGraphType::HYBRID_RAY_TRACING), rt_shadow_denoiser_settings.value());
	}

	// Auto-focus enabled
	if (arg_data.flagArgumentBool(DOF_AUTO_FOCUS_SHORT_FLAG, 1))
	{
		camera->m_focal_length = 0.0f;
	}

	return MStatus::kSuccess;
}

MSyntax wmr::RenderPipelineSelectCommand::create_syntax()
{
	MSyntax syntax;

	// Unsigned integers
	syntax.addFlag(PIPELINE_SHORT_FLAG, PIPELINE_LONG_FLAG, MSyntax::kUnsigned);
	syntax.addFlag(DOF_APERTURE_BLADE_COUNT_SHORT_FLAG, DOF_APERTURE_BLADE_COUNT_LONG_FLAG, MSyntax::kUnsigned);
	syntax.addFlag(RT_SHADOWS_SAMPLES_PER_PIXEL_SHORT_FLAG, RT_SHADOWS_SAMPLES_PER_PIXEL_LONG_FLAG, MSyntax::kUnsigned);

	// Strings
	syntax.addFlag(SKYBOX_SHORT_FLAG, SKYBOX_LONG_FLAG, MSyntax::kString);
	
	// Booleans
	syntax.addFlag(DOF_AUTO_FOCUS_SHORT_FLAG, DOF_AUTO_FOCUS_LONG_FLAG, MSyntax::kBoolean);
	syntax.addFlag(BLOOM_ENABLE_SHORT_FLAG, BLOOM_ENABLE_LONG_FLAG, MSyntax::kBoolean);
	syntax.addFlag(HBAO_BLUR_SHORT_FLAG, HBAO_BLUR_LONG_FLAG, MSyntax::kBoolean);
	syntax.addFlag(AS_DISABLE_REBUILD_SHORT_FLAG, AS_DISABLE_REBUILD_LONG_FLAG, MSyntax::kBoolean);

	// Doubles
	syntax.addFlag(DOF_FILM_SIZE_SHORT_FLAG, DOF_FILM_SIZE_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG, DOF_BOKEH_SHAPE_SIZE_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(HBAO_METERS_TO_UNITS_SHORT_FLAG, HBAO_METERS_TO_UNITS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(HBAO_RADIUS_SHORT_FLAG, HBAO_RADIUS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(HBAO_BIAS_SHORT_FLAG, HBAO_BIAS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(HBAO_POWER_SHORT_FLAG, HBAO_POWER_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(HBAO_BLUR_SHARPNESS_SHORT_FLAG, HBAO_BLUR_SHARPNESS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RTAO_BIAS_SHORT_FLAG, RTAO_BIAS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RTAO_RADIUS_SHORT_FLAG, RTAO_RADIUS_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RTAO_POWER_SHORT_FLAG, RTAO_POWER_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RTAO_SAMPLES_PER_PIXEL_SHORT_FLAG, RTAO_SAMPLES_PER_PIXEL_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_EPSILON_SHORT_FLAG, RT_SHADOWS_EPSILON_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_DENOISER_ALPHA_SHORT_FLAG, RT_SHADOWS_DENOISER_ALPHA_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_DENOISER_MOMENTS_ALPHA_SHORT_FLAG, RT_SHADOWS_DENOISER_MOMENTS_ALPHA_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG, RT_SHADOWS_DENOISER_L_PHI_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_DENOISER_N_PHI_SHORT_FLAG, RT_SHADOWS_DENOISER_N_PHI_LONG_FLAG, MSyntax::kDouble);
	syntax.addFlag(RT_SHADOWS_DENOISER_Z_PHI_SHORT_FLAG, RT_SHADOWS_DENOISER_Z_PHI_LONG_FLAG, MSyntax::kDouble);

	syntax.enableQuery(true);
	syntax.enableEdit(false);

	LOG("Finished creating custom MEL command syntax.");

	return syntax;
}
