#pragma once

// C++ standard
#include <array>

// Wisp rendering framework
namespace wr
{
	class FrameGraph;
	class RenderSystem;
}

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Available rendering pipelines
	/*! This enumeration is used to select different types of rendering "pipelines" when creating the Wisp frame graphs. */
	enum class RendererFrameGraphType
	{
		DEFERRED = 0,					/*!< Only use the basic deferred rendering pipeline. */
		HYBRID_RAY_TRACING,				/*!< Combine deferred rendering and ray tracing to render the scene. */
		FULL_RAY_TRACING,				/*!< Only use ray tracing to render the scene. */

		RENDERING_PIPELINE_TYPE_COUNT	/*!< Total number of rendering frame graph types available. */
	};

	//! Configures the render passes
	/*! Based on the frame graph type passed to the Create() function, render passes will be configured. */
	class FrameGraphManager
	{
	public:
		//! Unused
		FrameGraphManager() = default;
		
		//! Deallocate all frame graphs upon destruction
		/*! Whenever the frame graph manager goes out of scope, the existing frame graphs will be destroyed. There is no
		 *  need to keep track of the pointers in this class externally, the class will clean up after itself. */
		~FrameGraphManager();

		//! Create all frame graphs
		/*! Configure the render passes for all the frame graphs. For every single type in the RendererFrameGraphType
		 *  enumeration, a new frame graph is created.
		 *  
		 *  \param render_system Reference to the Wisp framework render system object.
		 *  \param initial_type Default rendering pipeline used in the application.
		 *  \sa RendererFrameGraphType */
		void Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type = RendererFrameGraphType::DEFERRED) noexcept;

		//! Set a frame graph type
		/*! Replace the currently active frame graph pipeline type with the new type.
		 *  
		 *  \param new_renderer_frame_graph_type Type of the new rendering pipeline.
		 *  \sa RendererFrameGraphType */
		void SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept;

		//! Get the currently active frame graph
		/*! Returns a pointer to the currently selected frame graph. Do not delete this pointer, just let this class
		 *  handle clean-up whenever it goes out of scope. */
		wr::FrameGraph* Get() const noexcept;

	private:
		//! Configure a frame graph for a full deferred rendering pipeline
		void CreateDeferredPipeline() noexcept;

		//! Configure a frame graph for a hybrid rendering pipeline
		void CreateHybridRTPipeline() noexcept;

		//! Configure a frame graph for a full ray traced rendering pipeline
		void CreateFullRTPipeline() noexcept;

	private:
		//! Currently selected rendering pipeline type
		RendererFrameGraphType m_current_rendering_pipeline_type;

		//! Container that holds all available frame graphs
		std::array<wr::FrameGraph*, static_cast<size_t>(RendererFrameGraphType::RENDERING_PIPELINE_TYPE_COUNT)> m_renderer_frame_graphs;
	};
}