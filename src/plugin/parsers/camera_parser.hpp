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
#include <maya/MApiNamespace.h>

// C++ standard
#include <memory>

namespace wr
{
	class CameraNode;
}

namespace wmr
{
	class CameraParser
	{
	public:
		CameraParser() = default;
		~CameraParser() = default;

		//! Fetches a reference to the renderer
		void Initialize();

		//! Set the Wisp camera to the currently active Maya camera
		void UpdateViewportCamera(const MString& panel_name);

	private:
		//! Pointer to the Wisp camera used to render the scene
		std::shared_ptr<wr::CameraNode> m_viewport_camera;
	};
}