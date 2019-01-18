#pragma once

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	class ViewportRenderer;

	class PluginMain
	{
	public:
		PluginMain() = default;
		~PluginMain() = default;

		void Initialize();
		void Uninitialize() const;

	private:
		bool IsSceneDirty() const;
		void ActOnCurrentDirtyState(const bool& state) const;

	private:
		std::unique_ptr<ViewportRenderer> m_wisp_viewport_renderer;
	};
}