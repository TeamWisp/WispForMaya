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

#pragma once

#include <maya/MPxCommand.h>

namespace wmr
{
	// Generic
	const constexpr char* PIPELINE_SHORT_FLAG = "-p";
	const constexpr char* SKYBOX_SHORT_FLAG = "-sb";

	const constexpr char* PIPELINE_LONG_FLAG = "-pipeline";
	const constexpr char* SKYBOX_LONG_FLAG = "-skybox";
	
	// Depth of field
	const constexpr char* DOF_FILM_SIZE_SHORT_FLAG = "-fs";
	const constexpr char* DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG = "-bss";
	const constexpr char* DOF_APERTURE_BLADE_COUNT_SHORT_FLAG = "-abc";
	const constexpr char* DOF_AUTO_FOCUS_SHORT_FLAG = "-af";

	const constexpr char* DOF_FILM_SIZE_LONG_FLAG = "-dof_film_size";
	const constexpr char* DOF_BOKEH_SHAPE_SIZE_LONG_FLAG = "-dof_bokeh_shape_size";
	const constexpr char* DOF_APERTURE_BLADE_COUNT_LONG_FLAG = "-dof_aperature_blade_count";
	const constexpr char* DOF_AUTO_FOCUS_LONG_FLAG = "-dof_auto_focus";
	
	// Bloom
	const constexpr char* BLOOM_ENABLE_SHORT_FLAG = "-be";
	const constexpr char* BLOOM_ENABLE_LONG_FLAG = "-enable_bloom";
	
	// NVIDIA HBAO
	const constexpr char* HBAO_METERS_TO_UNITS_SHORT_FLAG = "-hmu";
	const constexpr char* HBAO_RADIUS_SHORT_FLAG = "-hr";
	const constexpr char* HBAO_BIAS_SHORT_FLAG = "-hbi";
	const constexpr char* HBAO_POWER_SHORT_FLAG = "-hp";
	const constexpr char* HBAO_BLUR_SHORT_FLAG = "-hbl";
	const constexpr char* HBAO_BLUR_SHARPNESS_SHORT_FLAG = "-hbs";

	const constexpr char* HBAO_METERS_TO_UNITS_LONG_FLAG = "-hbao_meters_to_units";
	const constexpr char* HBAO_RADIUS_LONG_FLAG = "-hbao_radius";
	const constexpr char* HBAO_BIAS_LONG_FLAG = "-hbao_bias";
	const constexpr char* HBAO_POWER_LONG_FLAG = "-hbao_power";
	const constexpr char* HBAO_BLUR_LONG_FLAG = "-hbao_blur";
	const constexpr char* HBAO_BLUR_SHARPNESS_LONG_FLAG = "-hbao_blur_sharpness";

	// RTAO
	const constexpr char* RTAO_BIAS_SHORT_FLAG = "-rb";
	const constexpr char* RTAO_RADIUS_SHORT_FLAG = "-rr";
	const constexpr char* RTAO_POWER_SHORT_FLAG = "-rp";
	const constexpr char* RTAO_SAMPLES_PER_PIXEL_SHORT_FLAG = "-rs";

	const constexpr char* RTAO_BIAS_LONG_FLAG = "-rtao_bias";
	const constexpr char* RTAO_RADIUS_LONG_FLAG = "-rtao_radius";
	const constexpr char* RTAO_POWER_LONG_FLAG = "-rtao_power";
	const constexpr char* RTAO_SAMPLES_PER_PIXEL_LONG_FLAG = "-rtao_spp";

	// Ray-traced shadows
	const constexpr char* RT_SHADOWS_EPSILON_SHORT_FLAG = "-rse";
	const constexpr char* RT_SHADOWS_SAMPLES_PER_PIXEL_SHORT_FLAG = "-rss";
	const constexpr char* RT_SHADOWS_DENOISER_ALPHA_SHORT_FLAG = "-dna";
	const constexpr char* RT_SHADOWS_DENOISER_MOMENTS_ALPHA_SHORT_FLAG = "-dma";
	const constexpr char* RT_SHADOWS_DENOISER_L_PHI_SHORT_FLAG = "-dnl";
	const constexpr char* RT_SHADOWS_DENOISER_N_PHI_SHORT_FLAG = "-dnn";
	const constexpr char* RT_SHADOWS_DENOISER_Z_PHI_SHORT_FLAG = "-dnz";

	const constexpr char* RT_SHADOWS_EPSILON_LONG_FLAG = "-rt_shadows_epsilon";
	const constexpr char* RT_SHADOWS_SAMPLES_PER_PIXEL_LONG_FLAG = "-rt_shadows_spp";
	const constexpr char* RT_SHADOWS_DENOISER_ALPHA_LONG_FLAG = "-rt_shadows_denoise_alpha";
	const constexpr char* RT_SHADOWS_DENOISER_MOMENTS_ALPHA_LONG_FLAG = "-rt_shadows_denoise_moments_alpha";
	const constexpr char* RT_SHADOWS_DENOISER_L_PHI_LONG_FLAG = "-rt_shadows_denoise_l_phi";
	const constexpr char* RT_SHADOWS_DENOISER_N_PHI_LONG_FLAG = "-rt_shadows_denoise_n_phi";
	const constexpr char* RT_SHADOWS_DENOISER_Z_PHI_LONG_FLAG = "-rt_shadows_denoise_z_phi";

	// Acceleration structure
	const constexpr char* AS_DISABLE_REBUILD_SHORT_FLAG = "-dr";
	const constexpr char* AS_DISABLE_REBUILD_LONG_FLAG = "-disable_acceleration_structure_rebuilding";

	class Renderer;

	class RenderPipelineSelectCommand final : public MPxCommand
	{
	public:
		RenderPipelineSelectCommand() = default;
		virtual ~RenderPipelineSelectCommand() = default;

		// Virtual functions that *must* be overridden
		MStatus doIt(const MArgList& args) override;
		MStatus redoIt() override { return MStatus::kSuccess; }
		MStatus undoIt() override { return MStatus::kSuccess; }
		bool isUndoable() const override { return false; };
		static void* creator() { return new RenderPipelineSelectCommand(); }
		static MSyntax create_syntax();

	private:
	};
}