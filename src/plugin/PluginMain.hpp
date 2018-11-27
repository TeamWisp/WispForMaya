#pragma once

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	class WispViewportRenderer;

	class PluginMain
	{
	public:
		PluginMain();
		~PluginMain();

		void Initialize(std::unique_ptr<wmr::WispViewportRenderer>& t_viewport_renderer_override_instance) const;
		void Uninitialize(wmr::WispViewportRenderer* const t_viewport_renderer_override_instance) const;

	private:
		bool IsSceneDirty() const;

		void RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer, std::unique_ptr<WispViewportRenderer>& t_viewport_renderer_override_instance) const;
		void ThrowIfFailed(const MStatus& t_status) const;
		void UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer, WispViewportRenderer* const t_viewport_renderer_override_instance) const;
	};
}