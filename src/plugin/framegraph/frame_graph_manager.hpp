#pragma once

// C++ standard
#include <array>

// Wisp rendering framework
namespace wr
{
	class FrameGraph;
	class RenderSystem;
	class D3D12RenderSystem;
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
		PATH_TRACER,					/*!< Generate a 100% path traced. */

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
		 *  /param render_system Reference to the Wisp framework render system object.
		 *  /param initial_type Default rendering pipeline used in the application.
		 *  /param initial_width Width of the render texture.
		 *  /param initial_height Height of the render texture.
		 *  /sa RendererFrameGraphType */
		void Create(wr::RenderSystem& render_system, RendererFrameGraphType initial_type = RendererFrameGraphType::DEFERRED, std::uint32_t initial_width = 1280, std::uint32_t initial_height = 720) noexcept;

		//! Set a frame graph type
		/*! Replace the currently active frame graph pipeline type with the new type.
		 *  
		 *  /param new_renderer_frame_graph_type Type of the new rendering pipeline.
		 *  /sa RendererFrameGraphType */
		void SetType(RendererFrameGraphType new_renderer_frame_graph_type) noexcept;

		//! Get the currently active frame graph
		/*! Returns a pointer to the currently selected frame graph. Do not delete this pointer, just let this class
		 *  handle clean-up whenever it goes out of scope. */
		wr::FrameGraph* Get() const noexcept;

		//! Resize the current active frame graph */
		void Resize(unsigned int new_width, unsigned int new_height, wr::D3D12RenderSystem& render_system) noexcept;

		//! Retrieve the size of the frame graph
		/*! /return Pair containing the width and height respectively. */
		std::pair<std::uint32_t, std::uint32_t> GetCurrentDimensions() const noexcept;

	private:
		//! Configure a frame graph for a full deferred rendering pipeline
		void CreateDeferredPipeline() noexcept;

		//! Configure a frame graph for a hybrid rendering pipeline
		void CreateHybridRTPipeline() noexcept;

		//! Configure a frame graph for a full ray traced rendering pipeline
		void CreateFullRTPipeline() noexcept;

		//! Configure a frame graph for a full path traced rendering pipeline
		void CreatePathTracerPipeline() noexcept;

	private:
		std::uint32_t m_width;										//! Width of the render texture
		std::uint32_t m_height;										//! Height of the render texture
		RendererFrameGraphType m_current_rendering_pipeline_type;	//! Currently selected rendering pipeline type

		//! Container that holds all available frame graphs
		std::array<wr::FrameGraph*, static_cast<size_t>(RendererFrameGraphType::RENDERING_PIPELINE_TYPE_COUNT)> m_renderer_frame_graphs;
	};
}