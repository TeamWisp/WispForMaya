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
#include "util/log.hpp"

namespace wmr
{
	FrameGraphManager::~FrameGraphManager()
	{
		LOG("Starting render task destruction.");

		// Clean up the allocated frame graphs
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Destroy();
			delete frame_graph;
			frame_graph = nullptr;
		}

		LOG("Finished render task destruction.");
	}

	void FrameGraphManager::Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type, std::uint32_t initial_width, std::uint32_t initial_height) noexcept
	{
		LOG("Starting framegraph manager creation.");

		m_width = initial_width;
		m_height = initial_height;

		m_current_rendering_pipeline_type = initial_type;

		LOG("Starting to create all rendering pipelines.");

		// Add required tasks to each frame graph
		CreateDeferredPipeline();
		CreateHybridRTPipeline();
		CreateFullRTPipeline();
		CreatePathTracerPipeline();

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

	void FrameGraphManager::SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept
	{
		m_current_rendering_pipeline_type = new_renderer_frame_graph_type;
		
		switch (new_renderer_frame_graph_type)
		{
			case wmr::RendererFrameGraphType::DEFERRED:
				LOG("Current rendering pipeline switched to deferred.");
				break;
			
			case wmr::RendererFrameGraphType::HYBRID_RAY_TRACING:
				LOG("Current rendering pipeline switched to hybrid.");
				break;
			
			case wmr::RendererFrameGraphType::FULL_RAY_TRACING:
				LOG("Current rendering pipeline switched to full ray-tracing.");
				break;
			
			case wmr::RendererFrameGraphType::PATH_TRACER:
				LOG("Current rendering pipeline switched to path tracing.");
				break;
			
			case wmr::RendererFrameGraphType::RENDERING_PIPELINE_TYPE_COUNT:
				LOG("Current rendering pipeline switched to an invalid pipeline.");
				break;

			default:
				break;
		}
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

		// Wait until the GPU is done executing
		render_system.WaitForAllPreviousWork();

		// Resize the renderer viewport
		render_system.Resize(new_width, new_height);

		LOG("Resized the render system.");

		LOG("Starting to resize every framegraph...");

		// Resize all framegraphs
		for (auto& frame_graph : m_renderer_frame_graphs)
		{
			frame_graph->Resize(render_system, new_width, new_height);
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
		auto frame_graph = new wr::FrameGraph(9);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask(*frame_graph);
		LOG("Added BRDF LUT precalculation task.");

		// Skybox
		wr::AddEquirectToCubemapTask(*frame_graph);
		LOG("Added equirect to cubemap task.");

		wr::AddCubemapConvolutionTask(*frame_graph);
		LOG("Added cubemap convolution task.");

		// Construct the G-buffer
		wr::AddDeferredMainTask(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added deferred main task.");

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added depth data readback task.");

		// Merge the G-buffer into one final texture
		wr::AddDeferredCompositionTask(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added deferred composition task.");

		// Do some post processing
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*frame_graph);
		LOG("Added post-processing task.");

		// Save the final texture CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added pixel data readback task.");

		// Copy the composition pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);
		LOG("Added render target copy task.");

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::DEFERRED)] = frame_graph;

		LOG("Finished deferred pipeline creation.");
	}

	void FrameGraphManager::CreateHybridRTPipeline() noexcept
	{
		LOG("Starting hybrid pipeline creation.");
		auto frame_graph = new wr::FrameGraph(11);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask( *frame_graph );
		
		LOG("Added BRDF LUT precalculation task.");

		// Skybox
		wr::AddEquirectToCubemapTask( *frame_graph );
		LOG("Added equirect to cubemap task.");

		wr::AddCubemapConvolutionTask( *frame_graph );
		LOG("Added cubemap convolution task.");

		// Construct the G-buffer
		wr::AddDeferredMainTask(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added deferred main task.");

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added depth data readback task.");

		// Build acceleration structure
		wr::AddBuildAccelerationStructuresTask(*frame_graph);
		LOG("Added build acceleration structures task.");

		// Ray tracing
		wr::AddRTHybridTask(*frame_graph);
		LOG("Added RT-hybrid task.");

		wr::AddDeferredCompositionTask(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added deferred composition task.");

		// Do some post processing
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*frame_graph);
		LOG("Added post-processing task.");

		// Save the ray tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);
		LOG("Added pixel data readback task.");

		// Copy the ray tracing pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);
		LOG("Added render target copy task.");

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::HYBRID_RAY_TRACING)] = frame_graph;

		LOG("Finished hybrid pipeline creation.");
	}

	void FrameGraphManager::CreateFullRTPipeline() noexcept
	{
		auto frame_graph = new wr::FrameGraph(7);

		// Construct the acceleration structures needed for the ray tracing task
		wr::AddBuildAccelerationStructuresTask(*frame_graph);

		// Skybox
		wr::AddEquirectToCubemapTask(*frame_graph);
		wr::AddCubemapConvolutionTask(*frame_graph);

		// Perform ray tracing
		wr::AddRaytracingTask(*frame_graph);

		// Do some post processing
		wr::AddPostProcessingTask<wr::RaytracingData>(*frame_graph);

		// Save the ray tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);

		// Copy the scene render pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::FULL_RAY_TRACING)] = frame_graph;
	}

	void FrameGraphManager::CreatePathTracerPipeline() noexcept
	{
		auto frame_graph = new wr::FrameGraph(10);

		// Precalculate BRDF Lut
		wr::AddBrdfLutPrecalculationTask(*frame_graph);

		// Skybox
		wr::AddEquirectToCubemapTask(*frame_graph);
		wr::AddCubemapConvolutionTask(*frame_graph);

		// Construct the G-buffer
		wr::AddDeferredMainTask(*frame_graph, std::nullopt, std::nullopt);

		// Build Acceleration Structure
		wr::AddBuildAccelerationStructuresTask(*frame_graph);

		// Raytracing task
		//wr::AddRTHybridTask(*fg);

		// Global Illumination Path Tracing
		wr::AddPathTracerTask(*frame_graph);
		wr::AddAccumulationTask<wr::PathTracerData>(*frame_graph);

		wr::AddDeferredCompositionTask(*frame_graph, std::nullopt, std::nullopt);

		// Do some post processing
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*frame_graph);

		// Save the path tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);

		// Copy the raytracing pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);

		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::PATH_TRACER)] = frame_graph;
	}
}
