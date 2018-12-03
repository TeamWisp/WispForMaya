#include "PluginMain.hpp"
#include "overrides/ViewportRendererOverride.hpp"
#include "miscellaneous/Functions.hpp"

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>

void wmr::PluginMain::Initialize()
{
	// Workaround for avoiding dirtying the scene when registering overrides
	const auto is_scene_dirty = IsSceneDirty();

	CreateViewportRendererOverride();
	InitializeViewportRendererOverride();
	RegisterOverride();

	// If the scene was previously unmodified, return it to that state to avoid dirtying
	ActOnCurrentDirtyState(is_scene_dirty);
}

bool wmr::PluginMain::IsSceneDirty() const
{
	MStatus status = MStatus::kFailure;

	try
	{
		// Is the scene currently dirty?
		MCommandResult scene_dirty_result(&status);
		functions::ThrowIfFailedMaya(status);

		status = MGlobal::executeCommand("file -query -modified", scene_dirty_result);
		functions::ThrowIfFailedMaya(status);

		int command_result = -1;
		status = scene_dirty_result.getResult(command_result);
		functions::ThrowIfFailedMaya(status);

		return (command_result != 0);
	}
	catch (std::exception&)
	{
		return true;
	}
}

void wmr::PluginMain::CreateViewportRendererOverride()
{
	m_wisp_viewport_renderer = std::make_unique<WispViewportRenderer>("wisp_ViewportBlitOverride");
}

void wmr::PluginMain::InitializeViewportRendererOverride() const
{
	m_wisp_viewport_renderer->Initialize();
}

void wmr::PluginMain::RegisterOverride() const
{
	const auto maya_renderer = MHWRender::MRenderer::theRenderer();

	if (maya_renderer)
	{
		maya_renderer->registerOverride(m_wisp_viewport_renderer.get());
	}
}

void wmr::PluginMain::ActOnCurrentDirtyState(const bool& t_state) const
{
	// The scene is dirty, no need to set the flag
	if (!t_state)
	{
		MGlobal::executeCommand("file -modified 0");
	}
}

void wmr::PluginMain::Uninitialize() const
{
	const auto maya_renderer = MHWRender::MRenderer::theRenderer();

	if (maya_renderer)
	{
		maya_renderer->deregisterOverride(m_wisp_viewport_renderer.get());
	}
}
