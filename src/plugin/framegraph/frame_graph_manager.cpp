#include "frame_graph_manager.hpp"

// Wisp plug-in
#include "frame_graph/frame_graph.hpp"

// TODO: Find the best order of include in alphabetical order without breaking the dependencies
// Wisp rendering framework
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_raytracing_task.hpp"
#include "render_tasks/d3d12_rt_hybrid_task.hpp"
#include "render_tasks/d3d12_equirect_to_cubemap.hpp"
#include "render_tasks/d3d12_cubemap_convolution.hpp"
#include "render_tasks/d3d12_depth_data_readback.hpp"
#include "render_tasks/d3d12_pixel_data_readback.hpp"
#include "render_tasks/d3d12_post_processing.hpp"
#include "render_tasks/d3d12_build_acceleration_structures.hpp"

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

	void FrameGraphManager::Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type) noexcept
	{
		m_current_rendering_pipeline_type = initial_type;

		// Add required tasks to each frame graph
		CreateDeferredPipeline();
		CreateHybridRTPipeline();
		CreateFullRTPipeline();

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

	void FrameGraphManager::CreateDeferredPipeline() noexcept
	{
		auto frame_graph = new wr::FrameGraph(8);

		wr::AddEquirectToCubemapTask(*frame_graph);
		wr::AddCubemapConvolutionTask(*frame_graph);

		// Construct the G-buffer
		wr::AddDeferredMainTask(*frame_graph, std::nullopt, std::nullopt);

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*frame_graph, std::nullopt, std::nullopt);

		// Merge the G-buffer into one final texture
		wr::AddDeferredCompositionTask(*frame_graph, std::nullopt, std::nullopt);

		// Do some post processing
		wr::AddPostProcessingTask<wr::DeferredCompositionTaskData>(*frame_graph);

		// Save the final texture CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);

		// Copy the composition pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::DEFERRED)] = frame_graph;
	}

	void FrameGraphManager::CreateHybridRTPipeline() noexcept
	{
		auto frame_graph = new wr::FrameGraph(6);

		// Construct the G-buffer
		wr::AddDeferredMainTask(*frame_graph, std::nullopt, std::nullopt);

		// Save the depth buffer CPU pointer
		wr::AddDepthDataReadBackTask<wr::DeferredMainTaskData>(*frame_graph, std::nullopt, std::nullopt);

		// Build acceleration structure
		wr::AddBuildAccelerationStructuresTask(*frame_graph);

		// Ray tracing
		wr::AddRTHybridTask(*frame_graph);

		// Do some post processing
		wr::AddPostProcessingTask<wr::RTHybridData>(*frame_graph);

		// Save the ray tracing pixel data CPU pointer
		wr::AddPixelDataReadBackTask<wr::PostProcessingData>(*frame_graph, std::nullopt, std::nullopt);

		// Copy the ray tracing pixel data to the final render target
		wr::AddRenderTargetCopyTask<wr::PostProcessingData>(*frame_graph);

		// Store the frame graph for future use
		m_renderer_frame_graphs[static_cast<size_t>(RendererFrameGraphType::HYBRID_RAY_TRACING)] = frame_graph;
	}

	void FrameGraphManager::CreateFullRTPipeline() noexcept
	{
		auto frame_graph = new wr::FrameGraph(4);

		// Construct the acceleraion structures needed for the ray tracing task
		wr::AddBuildAccelerationStructuresTask(*frame_graph);

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
}
