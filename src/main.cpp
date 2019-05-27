// Wisp plug-in
#include "miscellaneous/functions.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/render_pipeline_select_command.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/callback_manager.hpp"
#include "util/log.hpp"

// Maya API
#include <maya/MCommandResult.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

// C++ standard
#include <direct.h>
#include <memory>
#include <filesystem>

wmr::ViewportRendererOverride* viewport_renderer_override;

class SetWorkDir{							
public:
	SetWorkDir()							
	{										
		_chdir( getenv( "WISP_MAYA" ) );	
	}										
};

static SetWorkDir set_work_dir;

// Global plug-in instance
bool IsSceneDirty()
{
	MStatus status = MStatus::kFailure;

	try
	{
		// Is the scene currently dirty?
		MCommandResult scene_dirty_result( &status );
		wmr::func::ThrowIfFailedMaya( status, "Could not find out whether the scene was dirty." );

		// Workaround for checking if the scene is, in fact, dirty
		status = MGlobal::executeCommand( "file -query -modified", scene_dirty_result );
		wmr::func::ThrowIfFailedMaya( status, "Could not change the file dirty status to not dirty." );

		int command_result = -1;
		status = scene_dirty_result.getResult( command_result );
		wmr::func::ThrowIfFailedMaya( status, "Could not get the result of the dirty command." );

		return ( command_result != 0 );
	}
	catch( std::exception& )
	{
		return true;
	}
}

void ActOnCurrentDirtyState( const bool& state )
{
	// The scene is dirty, no need to set the flag
	if( !state )
	{
		LOG("Scene was dirty, correcting...");
		MGlobal::executeCommand( "file -modified 0" );
	}
}

void LogCallback(std::string const& msg) {
	// CHANGE THIS ENTRY NUMBER IF THE LOCATION OF THE TYPE CHANGES
	// Current format:
	//   [hh:mm] [I] <msg>
	//            ^
	const int LOG_TYPE_LOC = 9;
	// Get character that defines the log type
	const char LOG_TYPE = msg[LOG_TYPE_LOC];
	// Print info on I
	if (LOG_TYPE == 'I') {
		MGlobal::displayInfo(msg.c_str());
	}
	// Print warning on W
	else if (LOG_TYPE == 'W') {
		MGlobal::displayWarning(msg.c_str());
	}
	// Print Error on E and C
	else if (LOG_TYPE == 'E' || LOG_TYPE == 'C') {
		MGlobal::displayError(msg.c_str());
	}
	// Display info if the type is unknown/invalid
	else {
		MGlobal::displayInfo(msg.c_str());
	}
}

//! Plug-in entry point
/*! Initializes the application. A plug-in object is created and stored. This object will hold the information Maya
 *  needs to make it all work. Once the plug-in object exists, the instance of the plug-in will be initialized, upon
 *  which lower-level systems will start working.
 *
 *  /param object Inherited function from Maya, see Autodesk documentation.
 *  /return Returns MStatus::kSucccess if everything went all right. */
MStatus initializePlugin(MObject object)
{
#ifndef _DEBUG
	std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto local_time = std::localtime(&current_time);
	std::stringstream ss;
	ss << "log-" << local_time->tm_hour << local_time->tm_min << "-" << local_time->tm_mday << "-" << (local_time->tm_mon + 1) << "-" << (local_time->tm_year + 1900);
	std::string log_file_name("WispForMaya.log");
	std::filesystem::path path = std::filesystem::path(ss.str());
	util::log_file_handler = new wr::LogfileHandler(path, log_file_name);
#endif // !_DEBUG

	LOG("Plugin initialization started.");

	util::log_callback::impl = std::function<void(std::string const&)>(LogCallback);
	LOG("Logging callback has been set.");

	// Register the plug-in to Maya, using the name and version data from the settings header file
	MFnPlugin plugin(object, wmr::settings::COMPANY_NAME, wmr::settings::PRODUCT_VERSION, "Any");

	// Register the command that enabled a MEL script to switch rendering pipelines
	plugin.registerCommand(wmr::settings::RENDER_PIPELINE_SELECT_COMMAND_NAME, wmr::RenderPipelineSelectCommand::creator, wmr::RenderPipelineSelectCommand::create_syntax);

	LOG("Registered Wisp menu custom command.");

	// Add the Wisp UI to the menu bar in Maya
	MGlobal::executeCommand("switch_rendering_pipeline");

	LOG("Wisp menu item has been added to the main menu.");

	// Workaround for avoiding dirtying the scene when registering overrides
	const auto is_scene_dirty = IsSceneDirty();

	// Initialize the renderer override
	viewport_renderer_override = new wmr::ViewportRendererOverride( wmr::settings::VIEWPORT_OVERRIDE_NAME );

	LOG("Registered Wisp renderer override.");

	// If the scene was previously unmodified, return it to that state to avoid dirtying
	ActOnCurrentDirtyState( is_scene_dirty );

	LOG("Finished plug-in initialization.");

	// If the program did not crash before this point, it means the plug-in was initialized correctly
	return MStatus::kSuccess;
}

//! Plug-in clean-up
/*  As soon as Maya tries to unload the plug-in, this function is called. The plug-in object is referenced and its
 *  destruction (called: "uninitialize") function is called. This will ensure a proper shut-down of all internal system.
 * 
 *  /param object Inherited function from Maya, see Autodesk documentation.
 *  /return Returns MStatus::kSucccess if everything went all right. */
MStatus uninitializePlugin(MObject object)
{
	LOG("Starting plug-in uninitialization.");
	
	MFnPlugin plugin(object);

	// Workaround for avoiding dirtying the scene when registering overrides
	const auto is_scene_dirty = IsSceneDirty();
	
	// Clean-up any used resources
	viewport_renderer_override->Destroy();

	delete viewport_renderer_override;
	wmr::CallbackManager::Destroy();

	// Remove the command used to add the Wisp UI to the menu bar
	plugin.deregisterCommand(wmr::settings::RENDER_PIPELINE_SELECT_COMMAND_NAME);

	// Remove the Wisp drop-down menu from the menu bar
	MGlobal::executeCommand("if(`menu -exists Wisp`) { deleteUI Wisp; }");

	// If the scene was previously unmodified, return it to that state to avoid dirtying
	ActOnCurrentDirtyState( is_scene_dirty );

	LOG("Finished plug-in uninitialization.");
#ifndef _DEBUG
	delete util::log_file_handler;
#endif // !_DEBUG

	// If the program did not crash before this point, the plug-in was uninitialized correctly
	return MStatus::kSuccess;
}
