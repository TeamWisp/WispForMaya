#include <maya/MFnPlugin.h>

#include "plugin/PluginMain.hpp"
#include "miscellaneous/Settings.hpp"

wmr::PluginMain plugin_instance;

MStatus initializePlugin(MObject t_object)
{
	MFnPlugin plugin(t_object, wisp::settings::COMPANY_NAME, wisp::settings::PRODUCT_VERSION, "Any");

	plugin_instance.Initialize();

	return MStatus::kSuccess;
}

MStatus uninitializePlugin(MObject t_object)
{
	MFnPlugin plugin(t_object);

	plugin_instance.Uninitialize();

	return MStatus::kSuccess;
}
