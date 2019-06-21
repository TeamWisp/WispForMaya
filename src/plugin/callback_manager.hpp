// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

		//! Destroy instance of CallbackManager
		static void Destroy();

		//! Register a callback
		/*! Used to registers callbacks using a MCallbackId structure.
		 *
		 *  /param msid Callback id. */
		void RegisterCallback(MCallbackId mcid);

		//! Unregisters a callback
		/*! Used to unregister callbacks using a MCallbackId structure.
		 *
		 *  /param msid Callback id. */
		void UnregisterCallback(MCallbackId mcid);

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
