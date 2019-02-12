#pragma once

#include <array>

namespace wr
{
	class FrameGraph;
	class RenderSystem;
}

namespace wmr
{
	enum class RendererFrameGraphType
	{
		DEFERRED = 0,
		HYBRID_RAY_TRACING,
		FULL_RAY_TRACING,

		RENDERING_PIPELINE_TYPE_COUNT
	};

	class FrameGraphManager
	{
	public:
		FrameGraphManager() = default;
		~FrameGraphManager();

		// Create all frame graphs and set the default type
		void Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type = RendererFrameGraphType::DEFERRED) noexcept;

		void SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept;
		wr::FrameGraph* Get() const noexcept;

	private:
		void CreateDeferredPipeline() noexcept;
		void CreateHybridRTPipeline() noexcept;
		void CreateFullRTPipeline() noexcept;

	private:
		RendererFrameGraphType m_current_rendering_pipeline_type;
		std::array<wr::FrameGraph*, static_cast<size_t>(RendererFrameGraphType::RENDERING_PIPELINE_TYPE_COUNT)> m_renderer_frame_graphs;
	};
}