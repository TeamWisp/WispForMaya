#pragma once

// Wisp Includes
#include <render_tasks/d3d12_rtao_task.hpp>
#include <render_tasks/d3d12_hbao.hpp>
#include <render_tasks/d3d12_ansel.hpp>
#include <render_tasks/d3d12_build_acceleration_structures.hpp>
#include <render_tasks/d3d12_rt_shadow_task.hpp>
#include <render_tasks/d3d12_shadow_denoiser_task.hpp>

// STD includes
#include <string>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Gets the settings by the templated typename
	/*! 
		\param frame_graph The framegraph to search in to gather the settings
		\return Returns an optional version of the settings. It may return `std::nullopt` if the settings couldn't be found in the provided framegraph
	*/
	template<typename T>
	inline std::optional<T> GetSettings(wr::FrameGraph* frame_graph)
	{
		// Ray Tracing Ambient Oclussion settings
		if constexpr (std::is_same<wr::RTAOSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::RTAOData>())
			{
				return frame_graph->GetSettings<wr::RTAOData, T>();
			}
		}
		// Horizon Based Ambient Oclussion settings
		else if constexpr (std::is_same<wr::HBAOSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::HBAOData>())
			{
				return frame_graph->GetSettings<wr::HBAOData, T>();
			}
		}
		// Acceleration Build settings
		else if constexpr (std::is_same<wr::ASBuildSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::ASBuildData>())
			{
				return frame_graph->GetSettings<wr::ASBuildData, T>();
			}
		}
		// Ray Tracing Shadow settings
		else if constexpr (std::is_same<wr::RTShadowSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::RTShadowData>())
			{
				return frame_graph->GetSettings<wr::RTShadowData, T>();
			}
		}
		// Ray Tracing Shadow Denoiser settings
		else if constexpr (std::is_same<wr::ShadowDenoiserSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::ShadowDenoiserData>())
			{
				return frame_graph->GetSettings<wr::ShadowDenoiserData, T>();
			}
		}

		return std::nullopt;
	}

	//! Sets the settings by the templated typename
	/*!
		\param frame_graph The framegraph to search in to set the settings
		\param settings The settings to set
	*/
	template<typename T>
	inline void SetSettings(wr::FrameGraph* frame_graph, T &settings)
	{
		// Ray Tracing Ambient Oclussion settings
		if constexpr (std::is_same<wr::RTAOSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::RTAOData>())
			{
				frame_graph->GetSettings<wr::RTAOData, T>();
			}
		}
		// Horizon Based Ambient Oclussion settings
		else if constexpr (std::is_same<wr::HBAOSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::HBAOData>())
			{
				frame_graph->GetSettings<wr::HBAOData, T>();
			}
		}
		// Acceleration Build settings
		else if constexpr (std::is_same<wr::ASBuildSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::ASBuildData>())
			{
				frame_graph->GetSettings<wr::ASBuildData, T>();
			}
		}
		// Ray Tracing Shadow settings
		else if constexpr (std::is_same<wr::RTShadowSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::RTShadowData>())
			{
				frame_graph->GetSettings<wr::RTShadowData, T>();
			}
		}
		// Ray Tracing Shadow Denoiser settings
		else if constexpr (std::is_same<wr::ShadowDenoiserSettings, T>::value)
		{
			if (frame_graph->HasTask<wr::ShadowDenoiserData>())
			{
				frame_graph->GetSettings<wr::ShadowDenoiserData, T>();
			}
		}
	}
}