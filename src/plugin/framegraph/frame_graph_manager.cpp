#include "frame_graph_manager.hpp"

// Wisp plug-in
#include "frame_graph/frame_graph.hpp"

// TODO: Find the best order of include in alphabetical order without breaking the dependencies
// Wisp rendering framework
#include "render_tasks/d3d12_brdf_lut_precalculation.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_depth_data_readback.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_pixel_data_readback.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_path_tracer.hpp"
#include "render_tasks/d3d12_accumulation.hpp"

#include "render_tasks/d3d12_dof_bokeh.hpp"
#include "render_tasks/d3d12_dof_bokeh_postfilter.hpp"
#include "render_tasks/d3d12_dof_coc.hpp"
#include "render_tasks/d3d12_down_scale.hpp"
#include "render_tasks/d3d12_dof_composition.hpp"
#include "render_tasks/d3d12_dof_dilate_near.hpp"
#include "render_tasks/d3d12_dof_dilate_flatten.hpp"
#include "render_tasks/d3d12_dof_dilate_flatten_second_pass.hpp"
#include "render_tasks/d3d12_hbao.hpp"
#include "render_tasks/d3d12_ansel.hpp"
#include "render_tasks/d3d12_bloom_composition.hpp"
#include "render_tasks/d3d12_bloom_horizontal.hpp"
#include "render_tasks/d3d12_bloom_vertical.hpp"

namespace wmr
{
	FrameGraphManager::~FrameGraphManager()
	{
		// Clean up the allocated frame graphs
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Destroy();
			delete frame_graph;
			frame_graph = nullptr;
		}
	}

	void FrameGraphManager::Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type, std::uint32_t initial_width, std::uint32_t initial_height) noexcept
	{
		m_width = initial_width;
		m_height = initial_height;

		m_current_rendering_pipeline_type = initial_type;

		// Add required tasks to each frame graph
		CreateDeferredPipeline();
		CreateHybridRTPipeline();

		// Set-up the rendering pipelines (frame graph configuration)
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Setup(render_system);
		}
	}

	void FrameGraphManager::SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept
	{
		m_current_rendering_pipeline_type = new_renderer_frame_graph_type;
	}

	wr::FrameGraph* FrameGraphManager::Get() const noexcept
	{
		return m_renderer_frame_graphs[static_cast<size_t>(m_current_rendering_pipeline_type)];
	}

	void FrameGraphManager::Resize(unsigned int new_width, unsigned int new_height, wr::D3D12RenderSystem& render_system) noexcept
	{
		m_width = new_width;
		m_height = new_height;

		// Wait until the GPU is done executing
		render_system.WaitForAllPreviousWork();

		// Resize the renderer viewport
		render_system.Resize(new_width, new_height);

		// Resize all framegraphs
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Resize(render_system, new_width, new_height);
		}
	}

	std::pair<std::uint32_t, std::uint32_t> FrameGraphManager::GetCurrentDimensions() const noexcept
	{
		return std::pair<std::uint32_t, std::uint32_t>(m_width, m_height);
	}

	void FrameGraphManager::CreateDeferredPipeline() noexcept
	{
		auto fg = new wr::FrameGraph(17);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask(*fg);

		// Skybox
		wr::AddEquirectToCubemapTask(*fg);
		wr::AddCubemapConvolutionTask(*fg);

		// Construct the G-buffer
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*fg, std::nullopt, std::nullopt);

		// Do HBAO
		wr::AddHBAOTask(*fg);

		// Merge the G-buffer into one final texture
		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

		// Do Depth of field task
		wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
		wr::AddDownScaleTask<wr::DeferredCompositionTaskData, wr::DoFCoCData>(*fg);
		wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
		wr::AddDoFDilateFlattenTask<wr::DoFDilateData>(*fg);
		wr::AddDoFDilateFlattenHTask<wr::DoFDilateFlattenData>(*fg);
		wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateFlattenHData>(*fg);
		wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);
		wr::AddDoFCompositionTask<wr::DeferredCompositionTaskData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);
		wr::AddBloomHorizontalTask<wr::DownScaleData>(*fg);
		wr::AddBloomVerticalTask<wr::BloomHData>(*fg);

		// Do some post processing//initialize default settings
		wr::BloomSettings defaultSettings;
		fg->UpdateSettings<wr::BloomSettings>(defaultSettings);

		wr::AddBloomCompositionTask<wr::DoFCompositionData, wr::BloomVData>(*fg);

		wr::AddPostProcessingTask<wr::BloomCompostionData>(*fg);

		// Save the final texture CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*fg, std::nullopt, std::nullopt);

		// Copy the composition pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::DEFERRED)] = fg;
	}

	void FrameGraphManager::CreateHybridRTPipeline() noexcept
	{
		auto fg = new wr::FrameGraph(18);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask( *fg);
		
		// Skybox
		wr::AddEquirectToCubemapTask( *fg);
		wr::AddCubemapConvolutionTask( *fg);

		// Construct the G-buffer
		wr::AddDeferredMainTask(*fg, std::nullopt, std::nullopt);

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*fg, std::nullopt, std::nullopt);

		// Build acceleration structure
		wr::AddBuildAccelerationStructuresTask(*fg);

		// Ray tracing
		wr::AddRTHybridTask(*fg);

		wr::AddDeferredCompositionTask(*fg, std::nullopt, std::nullopt);

		// Do Depth of field task
		wr::AddDoFCoCTask<wr::DeferredMainTaskData>(*fg);
		wr::AddDownScaleTask<wr::DeferredCompositionTaskData, wr::DoFCoCData>(*fg);
		wr::AddDoFDilateTask<wr::DownScaleData>(*fg);
		wr::AddDoFDilateFlattenTask<wr::DoFDilateData>(*fg);
		wr::AddDoFDilateFlattenHTask<wr::DoFDilateFlattenData>(*fg);
		wr::AddDoFBokehTask<wr::DownScaleData, wr::DoFDilateFlattenHData>(*fg);
		wr::AddDoFBokehPostFilterTask<wr::DoFBokehData>(*fg);
		wr::AddDoFCompositionTask<wr::DeferredCompositionTaskData, wr::DoFBokehPostFilterData, wr::DoFCoCData>(*fg);
		wr::AddBloomHorizontalTask<wr::DownScaleData>(*fg);
		wr::AddBloomVerticalTask<wr::BloomHData>(*fg);

		//initialize default settings
		wr::BloomSettings defaultSettings;
		fg->UpdateSettings<wr::BloomSettings>(defaultSettings);

		wr::AddBloomCompositionTask<wr::DoFCompositionData, wr::BloomVData>(*fg);

		// Do some post processing
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*fg);

		// Save the ray tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*fg, std::nullopt, std::nullopt);

		// Copy the ray tracing pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*fg);

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::HYBRID_RAY_TRACING)] = fg;
	}
}
