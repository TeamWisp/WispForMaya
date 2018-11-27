#include <maya/MFnPlugin.h>

#include "plugin/PluginMain.hpp"
#include "plugin/overrides/ViewportRendererOverride.hpp"
#include "miscellaneous/Settings.hpp"

#include <memory>

wmr::PluginMain plugin_instance;
std::unique_ptr<wmr::WispViewportRenderer> global_viewport_override_instance;

MStatus initializePlugin(MObject t_object)
{
	MFnPlugin plugin(t_object, wisp::settings::COMPANY_NAME, wisp::settings::PRODUCT_VERSION, "Any");

	plugin_instance.Initialize(global_viewport_override_instance);

	return MStatus::kSuccess;
}

MStatus uninitializePlugin(MObject t_object)
{
	MFnPlugin plugin(t_object);

	plugin_instance.Uninitialize(global_viewport_override_instance.get());

	return MStatus::kSuccess;
}
