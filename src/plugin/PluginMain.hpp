#pragma once

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	class PluginMain
	{
	public:
		PluginMain();
		~PluginMain();

		void Initialize() const;
		void Uninitialize() const;

	private:
		bool IsSceneDirty() const;

		void RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer) const;
		void ThrowIfFailed(const MStatus& t_status) const;
		void UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer) const;
	};
}