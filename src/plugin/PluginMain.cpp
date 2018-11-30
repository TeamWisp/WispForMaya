#include "PluginMain.hpp"
#include "overrides/ViewportRendererOverride.hpp"

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>

wmr::PluginMain::PluginMain()
{
}

wmr::PluginMain::~PluginMain()
{
}

void wmr::PluginMain::Initialize(std::unique_ptr<wmr::WispViewportRenderer>& t_viewport_renderer_override_instance) const
{
	// Workaround for avoiding dirtying the scene until there is a way to
	// register overrides without causing dirty.
	bool is_scene_dirty = true;
	is_scene_dirty = IsSceneDirty();

	RegisterPlugin(MHWRender::MRenderer::theRenderer(), t_viewport_renderer_override_instance);

	// If the scene was previously unmodified, return it to that state since
	// there are no changes that need to be saved
	if (is_scene_dirty == false)
	{
		MGlobal::executeCommand("file -modified 0");
	}
}

void wmr::PluginMain::Uninitialize(wmr::WispViewportRenderer* const t_viewport_renderer_override_instance) const
{
	UnregisterPlugin(MHWRender::MRenderer::theRenderer(), t_viewport_renderer_override_instance);
	t_viewport_renderer_override_instance->ShutDownRenderer();
}

bool wmr::PluginMain::IsSceneDirty() const
{
	MStatus status = MStatus::kFailure;

	try
	{
		// Is the scene currently dirty?
		MCommandResult scene_dirty_result(&status);
		ThrowIfFailed(status);

		status = MGlobal::executeCommand("file -query -modified", scene_dirty_result);
		ThrowIfFailed(status);

		int command_result = -1;
		status = scene_dirty_result.getResult(command_result);
		ThrowIfFailed(status);

		return (command_result != 0);
	}
	catch (std::exception&)
	{
		return true;
	}
}

void wmr::PluginMain::RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer, std::unique_ptr<WispViewportRenderer>& t_viewport_renderer_override_instance) const
{
	if (!t_maya_renderer)
	{
		return;
	}

	if (!t_viewport_renderer_override_instance)
	{
		t_viewport_renderer_override_instance = std::make_unique<wmr::WispViewportRenderer>("wisp_ViewportBlitOverride");
		t_maya_renderer->registerOverride(t_viewport_renderer_override_instance.get());
	}
}

void wmr::PluginMain::ThrowIfFailed(const MStatus& t_status) const
{
	if (t_status != MStatus::kSuccess)
	{
		throw std::exception();
	}
}

void wmr::PluginMain::UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer, WispViewportRenderer* const t_viewport_renderer_override_instance) const
{
	if (!t_maya_renderer)
	{
		return;
	}

	if (t_viewport_renderer_override_instance)
	{
		t_maya_renderer->deregisterOverride(t_viewport_renderer_override_instance);
	}
}
