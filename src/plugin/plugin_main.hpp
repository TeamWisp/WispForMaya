#pragma once

// Maya API
#include <maya/MViewport2Renderer.h>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	// Forward declarations
	class ViewportRenderer;

	//! Plugin main
	/*! Contains the entire plug-in. It is responsible for initializing and uninitializing the plug-in. */
	class PluginMain
	{
	public:
		//! Unused
		PluginMain() = default;

		//! Unused
		~PluginMain() = default;

		//! Initialize the plug-in
		/*! When called, the viewport renderer override is initialized and the plug-in is registered. */
		void Initialize();

		//! Uninitialize the plug-in
		/*! When called, the viewport renderer override is destroyed and the plug-in is unregistered. */
		void Uninitialize() const;

	private:
		//! Fix for Maya issues where the file is marked dirty for no reason
		/*! Queries the scene dirty state and executes a command to mark the scene as not dirty when needed. The code
		 *  originally came from the Autodesk API samples, so for more information and documentation, please have a look
		 *  at the SDK samples folder.
		 *  
		 *  \return bool True if the scene is dirty, else, false.
		 *  \sa ActOnCurrentDirtyState() */
		bool IsSceneDirty() const;

		//! Set the dirty file flag
		/*! If the file should be marked dirty, this function will execute the appropriate command to do this. It is
		 *  essentially a MEL command that is being executed from within C++.
		 *  
		 *  \param state Whether the current file should be marked dirty or not, use the result from IsSceneDirty().
		 *  \sa IsSceneDirty() */
		void ActOnCurrentDirtyState(const bool& state) const;

	private:
		//! Viewport renderer override
		std::unique_ptr<ViewportRenderer> m_wisp_viewport_renderer;
	};
}