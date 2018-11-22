#include "plugin/ViewportRendererOverride.hpp"

#include "Defines.hpp"

#include <maya/MFnPlugin.h>

#include <memory>

static std::unique_ptr<wisp::ViewportRendererOverride> renderer_override_instance;

void AddRemovePlugin(MStatus& t_status, bool t_add);
void CheckStatus(const MStatus& t_status, const MString& t_message);

// NOTE:	This function name deviates from the coding standard because Autodesk Maya looks for a function that is
//			written exactly like this.
MStatus initializePlugin(MObject t_object)
{
	MStatus status = MStatus::kFailure;
	MFnPlugin plugin(t_object, COMPANY_NAME, PRODUCT_VERSION);

	if (!renderer_override_instance)
	{
		AddRemovePlugin(status, true);
	}

	CheckStatus(status, "Failed to register the renderer override!");

	return status;
}

// NOTE:	This function name deviates from the coding standard because Autodesk Maya looks for a function that is
//			written exactly like this.
MStatus uninitializePlugin(MObject t_object)
{
	MStatus status = MStatus::kFailure;
	MFnPlugin plugin(t_object);

	// Unregister the override
	if (renderer_override_instance)
	{
		AddRemovePlugin(status, false);
	}

	CheckStatus(status, "Failed to unregister the renderer override!");

	return status;
}

void AddRemovePlugin(MStatus& t_status, bool t_add)
{
	auto renderer = MHWRender::MRenderer::theRenderer();

	if (renderer)
	{
		if (t_add)
		{
			// Create the renderer instance
			renderer_override_instance = std::make_unique<wisp::ViewportRendererOverride>(PRODUCT_NAME);

			// Register the renderer with Autodesk Maya
			t_status = renderer->registerOverride(renderer_override_instance.get());
		}
		else
		{
			// Unregister the renderer with Autodesk Maya
			t_status = renderer->deregisterOverride(renderer_override_instance.get());
		}
	}
}

void CheckStatus(const MStatus& t_status, const MString& t_message)
{
	if (!t_status)
	{
		t_status.perror(t_message);
	}
}
