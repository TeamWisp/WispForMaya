#include "PluginMain.hpp"
#include "overrides/ViewportRendererOverride.hpp"
#include "miscellaneous/Functions.hpp"

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>

namespace wmr
{
	void PluginMain::Initialize()
	{
		// Workaround for avoiding dirtying the scene when registering overrides
		const auto is_scene_dirty = IsSceneDirty();

		CreateViewportRendererOverride();
		InitializeViewportRendererOverride();
		RegisterOverride();

		// If the scene was previously unmodified, return it to that state to avoid dirtying
		ActOnCurrentDirtyState(is_scene_dirty);
	}

	bool PluginMain::IsSceneDirty() const
	{
		MStatus status = MStatus::kFailure;

		try
		{
			// Is the scene currently dirty?
			MCommandResult scene_dirty_result(&status);
			func::ThrowIfFailedMaya(status);

			// Workaround for checking if the scene is, in fact, dirty
			status = MGlobal::executeCommand("file -query -modified", scene_dirty_result);
			func::ThrowIfFailedMaya(status);

			int command_result = -1;
			status = scene_dirty_result.getResult(command_result);
			func::ThrowIfFailedMaya(status);

			return (command_result != 0);
		}
		catch (std::exception&)
		{
			return true;
		}
	}

	void PluginMain::CreateViewportRendererOverride()
	{
		m_wisp_viewport_renderer = std::make_unique<WispViewportRenderer>("wisp_ViewportBlitOverride");
	}

	void PluginMain::InitializeViewportRendererOverride() const
	{
		m_wisp_viewport_renderer->Initialize();
	}

	void PluginMain::RegisterOverride() const
	{
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (maya_renderer)
		{
			maya_renderer->registerOverride(m_wisp_viewport_renderer.get());
		}
	}

	void PluginMain::ActOnCurrentDirtyState(const bool& state) const
	{
		// The scene is dirty, no need to set the flag
		if (!state)
		{
			MGlobal::executeCommand("file -modified 0");
		}
	}

	void PluginMain::UninitializeViewportRendererOverride() const
	{
		m_wisp_viewport_renderer->Destroy();
	}

	void PluginMain::Uninitialize() const
	{
		// This makes sure the plug-in itself can be deinitialized
		UninitializeViewportRendererOverride();

		// Not the Wisp renderer, but the internal Maya renderer
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (maya_renderer)
		{
			// Deregister the actual plug-in
			maya_renderer->deregisterOverride(m_wisp_viewport_renderer.get());
		}
	}
}
