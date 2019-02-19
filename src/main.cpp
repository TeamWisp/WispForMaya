// Wisp plug-in
#include "miscellaneous/settings.hpp"
#include "plugin/plugin_main.hpp"
#include "plugin/viewport_renderer.hpp"

// Maya API
#include <maya/MFnPlugin.h>

// C++ standard
#include <memory>

// Global plug-in instance
wmr::PluginMain plugin_instance;

//! Plug-in entry point
/*! Initializes the application. A plug-in object is created and stored. This object will hold the information Maya
 *  needs to make it all work. Once the plug-in object exists, the instance of the plug-in will be initialized, upon
 *  which lower-level systems will start working.
 *
 *  \param object Inherited function from Maya, see Autodesk documentation.
 *  \return Returns MStatus::kSucccess if everything went all right. */
MStatus initializePlugin(MObject object)
{
	// Register the plug-in to Maya, using the name and version data from the settings header file
	MFnPlugin plugin(object, wmr::settings::COMPANY_NAME, wmr::settings::PRODUCT_VERSION, "Any");

	// Initialization of the plug-in
	plugin_instance.Initialize();

	// If the program did not crash before this point, it means the plug-in was initialized correctly
	return MStatus::kSuccess;
}

//! Plug-in clean-up
/*  As soon as Maya tries to unload the plug-in, this function is called. The plug-in object is referenced and its
 *  destruction (called: "uninitialize") function is called. This will ensure a proper shut-down of all internal system.
 * 
 *  \param object Inherited function from Maya, see Autodesk documentation.
 *  \return Returns MStatus::kSucccess if everything went all right. */
MStatus uninitializePlugin(MObject object)
{
	MFnPlugin plugin(object);

	// Clean-up any used resources
	plugin_instance.Uninitialize();

	// If the program did not crash before this point, the plug-in was uninitialized correctly
	return MStatus::kSuccess;
}
