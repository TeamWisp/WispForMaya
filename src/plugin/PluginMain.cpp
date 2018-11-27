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

void wmr::PluginMain::Initialize() const
{
	// Workaround for avoiding dirtying the scene until there is a way to
	// register overrides without causing dirty.
	bool is_scene_dirty = true;
	is_scene_dirty = IsSceneDirty();

	RegisterPlugin(MHWRender::MRenderer::theRenderer());

	// If the scene was previously unmodified, return it to that state since
	// there are no changes that need to be saved
	if (is_scene_dirty == false)
	{
		MGlobal::executeCommand("file -modified 0");
	}
}

void wmr::PluginMain::Uninitialize() const
{
	UnregisterPlugin(MHWRender::MRenderer::theRenderer());
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

void wmr::PluginMain::RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer) const
{
	if (!t_maya_renderer)
	{
		return;
	}

	if (!wmr::WispViewportRenderer::global_viewport_renderer_instance)
	{
		wmr::WispViewportRenderer::global_viewport_renderer_instance = std::make_unique<wmr::WispViewportRenderer>("my_viewImageBlitOverride");
		t_maya_renderer->registerOverride(wmr::WispViewportRenderer::global_viewport_renderer_instance.get());
	}
}

void wmr::PluginMain::ThrowIfFailed(const MStatus& t_status) const
{
	if (t_status != MStatus::kSuccess)
	{
		throw std::exception();
	}
}

void wmr::PluginMain::UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer) const
{
	if (!t_maya_renderer)
	{
		return;
	}

	if (wmr::WispViewportRenderer::global_viewport_renderer_instance)
	{
		t_maya_renderer->deregisterOverride(wmr::WispViewportRenderer::global_viewport_renderer_instance.get());
	}
}
