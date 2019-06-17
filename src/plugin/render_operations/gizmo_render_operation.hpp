// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zijnstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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
#include <maya/MShaderManager.h>

// Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! User-interface override implementation
	/*! Implementation of a Maya MSceneRender. It inherits from the Maya API base class and implements all methods needed
	 *  to make the override work. The code style for functions is a bit different here because our style guide differs
	 *  from the style used for the Maya API. */
	class GizmoRenderOperation final : public MHWRender::MSceneRender
	{
	public:
		//! Sets the name of the base class to the argument value
		GizmoRenderOperation(const MString& name);

		//! Unused
		~GizmoRenderOperation() override = default;

	private:
		//! Filter used to render the user-interface
		/*! Please note that this is an implementation of a Maya API function. Please refer to the Autodesk documentation
		 *  for more information.
		 *  
		 *  /return Scene renderer filter type. */
		MHWRender::MSceneRender::MSceneFilterOption renderFilterOverride() override;

		//! Configure the clear operation for the user-interface renderer
		/*! Please note that this is a function override from the Maya API, so check the Autodesk documentation for more
		 *  information.
		 *
		 *  /return Returns the clear operation data structure as seen in the Maya API. */
		MHWRender::MClearOperation& clearOperation() override;
		
		//! Configure what the renderer should render
		/*! Please note that this is an implementation of a Maya API function. Please refer to the Autodesk documentation
		 *  for more information.
		 *  
		 *  return Flag value, check the Autodesk documentation for specific values that can be used here. */
		MUint64 getObjectTypeExclusions() override;
	};
}