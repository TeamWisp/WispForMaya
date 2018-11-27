#include <stdio.h>
#include <exception>

#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>

#include "Settings.hpp"
#include "plugin/ViewportRendererOverride.hpp"

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, wisp::settings::COMPANY_NAME, wisp::settings::PRODUCT_VERSION, "Any");

	// Workaround for avoiding dirtying the scene until there is a way to
	// register overrides without causing dirty.
	bool sceneDirty = true;

	try
	{
		// Is the scene currently dirty?
		MCommandResult sceneDirtyResult(&status);
		if (status != MStatus::kSuccess) throw std::exception();
		status = MGlobal::executeCommand("file -query -modified", sceneDirtyResult);
		if (status != MStatus::kSuccess) throw std::exception();
		int commandResult;
		status = sceneDirtyResult.getResult(commandResult);
		if (status != MStatus::kSuccess) throw std::exception();
		sceneDirty = commandResult != 0;
	}
	catch (std::exception&)
	{
		sceneDirty = true;
	}

	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (renderer)
	{
		if (!wmr::WispViewportRenderer::sViewImageBlitOverrideInstance)
		{
			wmr::WispViewportRenderer::sViewImageBlitOverrideInstance = new wmr::WispViewportRenderer("my_viewImageBlitOverride");
			renderer->registerOverride(wmr::WispViewportRenderer::sViewImageBlitOverrideInstance);
		}
	}

	// If the scene was previously unmodified, return it to that state since
	// there are no changes that need to be saved
	if (sceneDirty == false)
	{
		MGlobal::executeCommand("file -modified 0");
	}

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	if (renderer)
	{
		if (wmr::WispViewportRenderer::sViewImageBlitOverrideInstance)
		{
			renderer->deregisterOverride(wmr::WispViewportRenderer::sViewImageBlitOverrideInstance);
			delete wmr::WispViewportRenderer::sViewImageBlitOverrideInstance;
		}
		wmr::WispViewportRenderer::sViewImageBlitOverrideInstance = nullptr;
	}

	return status;
}

