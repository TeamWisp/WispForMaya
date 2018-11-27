#include <stdio.h>
#include <exception>

#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>

#include "Settings.hpp"
#include "plugin/ViewportRendererOverride.hpp"

namespace wmr::util
{
	void ThrowIfFailed(const MStatus& t_status);
	void RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer);
	void UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer);

	bool IsSceneDirty();
}

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, wisp::settings::COMPANY_NAME, wisp::settings::PRODUCT_VERSION, "Any");

	// Workaround for avoiding dirtying the scene until there is a way to
	// register overrides without causing dirty.
	bool is_scene_dirty = true;
	is_scene_dirty = wmr::util::IsSceneDirty();

	wmr::util::RegisterPlugin(MHWRender::MRenderer::theRenderer());

	// If the scene was previously unmodified, return it to that state since
	// there are no changes that need to be saved
	if (is_scene_dirty == false)
	{
		MGlobal::executeCommand("file -modified 0");
	}

	return MStatus::kSuccess;
}

bool wmr::util::IsSceneDirty()
{
	MStatus status = MStatus::kFailure;

	try
	{
		// Is the scene currently dirty?
		MCommandResult scene_dirty_result(&status);
		wmr::util::ThrowIfFailed(status);

		status = MGlobal::executeCommand("file -query -modified", scene_dirty_result);
		wmr::util::ThrowIfFailed(status);

		int command_result = -1;
		status = scene_dirty_result.getResult(command_result);
		wmr::util::ThrowIfFailed(status);

		return (command_result != 0);
	}
	catch (std::exception&)
	{
		return true;
	}
}

void wmr::util::ThrowIfFailed(const MStatus& t_status)
{
	if (t_status != MStatus::kSuccess)
	{
		throw std::exception();
	}
}

void wmr::util::RegisterPlugin(MHWRender::MRenderer* const t_maya_renderer)
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

MStatus uninitializePlugin(MObject t_object)
{
	MFnPlugin plugin(t_object);

	wmr::util::UnregisterPlugin(MHWRender::MRenderer::theRenderer());

	return MStatus::kSuccess;
}

void wmr::util::UnregisterPlugin(MHWRender::MRenderer* const t_maya_renderer)
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
