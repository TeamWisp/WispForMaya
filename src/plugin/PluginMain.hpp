#pragma once

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	class WispViewportRenderer;

	class PluginMain
	{
	public:
		PluginMain() = default;
		~PluginMain() = default;

		void Initialize();
		void Uninitialize() const;

	private:
		bool IsSceneDirty() const;
		
		void ThrowIfFailed(const MStatus& t_status) const;
		void CreateViewportRendererOverride();
		void RegisterOverride() const;
		void ActOnCurrentDirtyState(const bool& t_state) const;

	private:
		std::unique_ptr<WispViewportRenderer> m_wisp_viewport_renderer;
	};
}