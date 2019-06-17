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

#include "frame_graph_manager.hpp"

// Wisp plug-in
#include "frame_graph/frame_graph.hpp"

// TODO: Find the best order of include in alphabetical order without breaking the dependencies
// Wisp rendering framework
#include "frame_graph/frame_graph.hpp"
#include "settings.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_reflection_task.hpp"
#include "render_tasks/d3d12_rt_shadow_task.hpp"
#include "render_tasks/d3d12_shadow_denoiser_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_rtao_task.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"
#include "render_tasks/d3d12_dof_bokeh.hpp"
#include "render_tasks/d3d12_dof_bokeh_postfilter.hpp"
#include "render_tasks/d3d12_dof_coc.hpp"
#include "render_tasks/d3d12_down_scale.hpp"
#include "render_tasks/d3d12_dof_composition.hpp"
#include "render_tasks/d3d12_dof_dilate_near.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_bloom_extract_bright.hpp"
#include "render_tasks/d3d12_bloom_composition.hpp"
#include "render_tasks/d3d12_bloom_half_res.hpp"
#include "render_tasks/d3d12_bloom_half_res_v.hpp"
#include "render_tasks/d3d12_bloom_quarter_res.hpp"
#include "render_tasks/d3d12_bloom_quarter_res_v.hpp"
#include "render_tasks/d3d12_bloom_eighth_res.hpp"
#include "render_tasks/d3d12_bloom_eighth_res_v.hpp"
#include "render_tasks/d3d12_bloom_sixteenth_res.hpp"
#include "render_tasks/d3d12_bloom_sixteenth_res_v.hpp"

#include "wisp_render_tasks/d3d12_depth_data_readback.hpp"
#include "wisp_render_tasks/d3d12_pixel_data_readback.hpp"

namespace wmr
{
	void FrameGraphManager::Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type, std::uint32_t initial_width, std::uint32_t initial_height) noexcept
	{
		LOG("Starting framegraph manager creation.");

		m_width = initial_width;
		m_height = initial_height;

		m_current_rendering_pipeline_type = initial_type;

		LOG("Starting to create all rendering pipelines.");

		// Add required tasks to each frame graph
		CreateDeferredPipeline();
		CreateHybridRTPipeline(render_system);

		LOG("Finished creating all rendering pipelines.");

		LOG("Finalizing framegraphs...");

		// Set-up the rendering pipelines (frame graph configuration)
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Setup(render_system);
		}

		LOG("Finished finalizing framegraphs.");

		LOG("Finished framegraph manager creation.");
	}

	void FrameGraphManager::Destroy() noexcept
	{
		// Clean up the allocated frame graphs
		for (auto* frame_graph : m_renderer_frame_graphs)
		{
			// Not allocated
			if (!frame_graph)
				continue;

			frame_graph->Destroy();
			delete frame_graph;
			frame_graph = nullptr;
		}
	}

	void FrameGraphManager::SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept
	{
		switch (new_renderer_frame_graph_type)
		{
			case wmr::RendererFrameGraphType::DEFERRED:
				LOG("Current rendering pipeline switched to deferred.");
				break;
			
			case wmr::RendererFrameGraphType::HYBRID_RAY_TRACING:
				LOG("Current rendering pipeline switched to hybrid.");
				break;
			
			default:
				LOG("Current rendering pipeline switched to an invalid pipeline.");
				return;	// Invalid type
		}
		
		m_current_rendering_pipeline_type = new_renderer_frame_graph_type;
	}

	wr::FrameGraph* FrameGraphManager::Get() const noexcept
	{
		return m_renderer_frame_graphs[static_cast<size_t>(m_current_rendering_pipeline_type)];
	}

	void FrameGraphManager::Resize(unsigned int new_width, unsigned int new_height, wr::D3D12RenderSystem& render_system) noexcept
	{
		LOG("Starting framegraph resizing");

		m_width = new_width;
		m_height = new_height;

		LOG("New framegraph size: {}x{} pixels.", new_width, new_height);

		// Wait until the GPU is done executing
		render_system.WaitForAllPreviousWork();

		// Resize the renderer viewport
		render_system.Resize(new_width, new_height);

		LOG("Resized the render system.");

		LOG("Starting to resize every framegraph...");

		// Resize all framegraphs
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Resize( new_width, new_height);
		}

		LOG("Resized all framegraphs successfully.");
	}

	std::pair<std::uint32_t, std::uint32_t> FrameGraphManager::GetCurrentDimensions() const noexcept
	{
		return std::pair<std::uint32_t, std::uint32_t>(m_width, m_height);
	}

	void FrameGraphManager::CreateDeferredPipeline() noexcept
	{
		LOG("Starting deferred pipeline creation.");
		auto fg = new wr::FrameGraph(17);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask(*fg);
		LOG("Added BRDF LUT precalculation task.");

		// Skybox
		wr::AddEquirectToCubemapTask(*fg);
		LOG("Added equirect to cubemap task.");
		wr::AddCubemapConvolutionTask(*fg);
		LOG("Added cubemap convolution task.");

		// Construct the G-buffer
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, false);
		LOG("Added deferred main task.");

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*fg, std::nullopt, std::nullopt);
		LOG("Added depth data readback task.");

		// Do HBAO
		wr::AddHBAOTask(*fg);
		LOG("Added HBAO.");

		// Merge the G-buffer into one final texture
		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
		LOG("Added deferred composition task.");

		// High quality bloom pass
		wr::AddBloomExtractBrightTask<wr::DeferredCompositionTaskData, wr::DeferredMainTaskData>(*fg);
		wr::AddBloomHalfTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomHalfVTask<wr::BloomHalfData>(*fg);
		wr::AddBloomQuarterTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomQuarterVTask<wr::BloomQuarterData>(*fg);
		wr::AddBloomEighthTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomEighthVTask<wr::BloomEighthData>(*fg);
		wr::AddBloomSixteenthTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomSixteenthVTask<wr::BloomSixteenthData>(*fg);
		wr::AddBloomCompositionTask<wr::DeferredCompositionTaskData, wr::BloomHalfVData, wr::BloomQuarterVData, wr::BloomEighthVData, wr::BloomSixteenthVData>(*fg);
		LOG("Added high quality bloom task.");

		// Do Depth of field task
		wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
		wr::AddDownScaleTask<wr::BloomCompostionData, wr::DoFCoCData>(*fg);
		wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
		wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateData>(*fg);
		wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);
		wr::AddDoFCompositionTask<wr::BloomCompostionData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);
		LOG("Added depth of field task.");

		wr::AddPostProcessingTask<wr::DoFCompositionData>(*fg);
		LOG("Added post-processing task.");

		// Save the final texture CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*fg, std::nullopt, std::nullopt);
		LOG("Added pixel data readback task.");

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::DEFERRED)] = fg;

		LOG("Finished deferred pipeline creation.");
	}

	void FrameGraphManager::CreateHybridRTPipeline(wr::RenderSystem& render_system) noexcept
	{
		LOG("Starting hybrid pipeline creation.");
		auto fg = new wr::FrameGraph(18);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask( *fg);
		LOG("Added BRDF LUT precalculation task.");

		// Skybox
		wr::AddEquirectToCubemapTask( *fg);
		LOG("Added equirect to cubemap task.");
		wr::AddCubemapConvolutionTask( *fg);
		LOG("Added cubemap convolution task.");

		// Construct the G-buffer
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt, true);
		LOG("Added deferred main task.");

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*fg, std::nullopt, std::nullopt);
		LOG("Added depth data readback task.");

		// Build acceleration structure
		wr::AddBuildAccelerationStructuresTask(*fg);
		LOG("Added build acceleration structures task.");

		// Ray tracing
		wr::AddRTReflectionTask(*fg);
		LOG("Added raytraced reflections task.");
		wr::AddRTShadowTask(*fg);
		LOG("Added raytraced shadows task.");

		wr::AddShadowDenoiserTask(*fg);
		LOG("Added shadow denoiser task.");

		wr::AddRTAOTask(*fg, static_cast<wr::D3D12RenderSystem&>(render_system).m_device);
		LOG("Added raytraced ambient occlussion task.");

		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);
		LOG("Added deferred composition task.");

		// High quality bloom pass
		wr::AddBloomExtractBrightTask<wr::DeferredCompositionTaskData, wr::DeferredMainTaskData>(*fg);
		wr::AddBloomHalfTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomHalfVTask<wr::BloomHalfData>(*fg);
		wr::AddBloomQuarterTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomQuarterVTask<wr::BloomQuarterData>(*fg);
		wr::AddBloomEighthTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomEighthVTask<wr::BloomEighthData>(*fg);
		wr::AddBloomSixteenthTask<wr::BloomExtractBrightData>(*fg);
		wr::AddBloomSixteenthVTask<wr::BloomSixteenthData>(*fg);
		wr::AddBloomCompositionTask<wr::DeferredCompositionTaskData, wr::BloomHalfVData, wr::BloomQuarterVData, wr::BloomEighthVData, wr::BloomSixteenthVData>(*fg);
		LOG("Added high quality bloom task.");

		// Do Depth of field task
		wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
		wr::AddDownScaleTask<wr::BloomCompostionData, wr::DoFCoCData>(*fg);
		wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
		wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateData>(*fg);
		wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);
		wr::AddDoFCompositionTask<wr::BloomCompostionData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);
		LOG("Added depth of field task.");

		// Do some post processing
		wr::AddPostProcessingTask<wr::DoFCompositionData>(*fg);
		LOG("Added post-processing task.");

		// Save the ray tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*fg, std::nullopt, std::nullopt);
		LOG("Added pixel data readback task.");

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::HYBRID_RAY_TRACING)] = fg;

		LOG("Finished hybrid pipeline creation.");
	}
}
