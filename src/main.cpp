#include <maya/MFnPlugin.h>

#include "plugin/PluginMain.hpp"
#include "plugin/overrides/ViewportRendererOverride.hpp"
#include "miscellaneous/Settings.hpp"

#include <memory>

wmr::PluginMain plugin_instance;

MStatus initializePlugin(MObject object)
{
	MFnPlugin plugin(object, wisp::settings::COMPANY_NAME, wisp::settings::PRODUCT_VERSION, "Any");

	plugin_instance.Initialize();

	return MStatus::kSuccess;
}

MStatus uninitializePlugin(MObject object)
{
	MFnPlugin plugin(object);

	plugin_instance.Uninitialize();

	return MStatus::kSuccess;
}
