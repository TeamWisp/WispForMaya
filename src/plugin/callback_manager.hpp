#pragma once

// Maya API
#include <maya/MMessage.h>

// C++ standard
#include <vector>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Centralized place to manage the callbacks used throughout this application
	/*! This class is responsible for registering and unregistering any callbacks needed by the application. All callbacks
	 *  provided by Maya should be registered in here. */
	class CallbackManager
	{
	public:
		CallbackManager() = default;

		//! Automatically reset the callback manager upon destruction
		/*! When the callback manager is destroyed, the destructor always calls Reset() to make sure every callback has
		 *  been reset before the plug-in quits.
		 *  
		 *  /sa Reset()*/
		~CallbackManager();

		//! Get a hold of the Singleton instance
		/*! /return Returns a reference to the callback manager if it exists, else, nullptr. */
		static CallbackManager& GetInstance();

		//! Register a callback
		/*! Used to registers callbacks using a MCallbackId structure.
		 *
		 *  /param msid Callback id. */
		void RegisterCallback(MCallbackId mcid);

		//! Reset the callback manager
		/*! If any callbacks have been set, this function will make sure that they are properly disposed of.
		 *  
		 *  /sa ~CallbackManager()*/
		void Reset();

	private:
		//! Singleton instance
		static CallbackManager* m_instance;

		//! Callback ID container
		/*! Hold all callbacks registered to the callback manager. */
		std::vector<MCallbackId> m_callback_vector;

	};
}
