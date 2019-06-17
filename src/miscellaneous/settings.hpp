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

// Wisp plug-in
#include "miscellaneous/functions.hpp"

// C++ standard
#include <array>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Holds all global settings for the plug-in
	namespace settings
	{
		//! Maximum amount of vertex data in the model pool in MB
		static const constexpr std::uint32_t MAX_VERTEX_DATA_SIZE_MB = 64MB;

		//! Maximum amount of index data in the model pool in MB
		static const constexpr std::uint32_t MAX_INDEX_DATA_SIZE_MB = 8MB;

		//! Name of the studio / company developing this product
		static const constexpr char* COMPANY_NAME = "Team Wisp";

		//! Name of the product itself
		static const constexpr char* PRODUCT_NAME = "Wisp";
		
		//! Current release version of the product
		static const constexpr char* PRODUCT_VERSION = "0.2.0";
		
		//! Name of the viewport override
		static const constexpr char* VIEWPORT_OVERRIDE_NAME = "wisp_viewport_override";

		//! Meshes with this prefix will be ignored by the parsers (some kind of internal Autodesk Maya name prefix for intermediate objects when duplicating)
		static const constexpr char* INTERMEDIATE_MESH_IGNORE_PREFIX = "__PrenotatoPerDuplicare_";

		static const constexpr std::uint32_t RENDER_OPERATION_COUNT = 7;

		//! Names and order of the render operations in this plug-in
		static const constexpr std::array<const char*, RENDER_OPERATION_COUNT> RENDER_OPERATION_NAMES =
		{
			"wisp_renderer_update",	// Update the Wisp rendering framework				(renderer_update_operation.hpp)
			"wisp_renderer_draw",	// Render using Wisp render textures				(renderer_draw_operation.hpp)
			"wisp_renderer_copy",	// Copy the Wisp output into Maya render textures	(renderer_copy_operation.hpp)
			"wisp_fullscreen_blit",	// Blit the Maya render textures to the screen		(screen_render_operation.hpp)
			"wisp_gizmo_render",	// Render gizmos									(gizmo_render_operation.hpp)
			"wisp_present"			// Present the results to the viewport window		(using default Maya implementation)
		};

		//! Name of the command used to switch rendering pipelines
		static const constexpr char* CUSTOM_WISP_UI_CMD = "wisp_handle_ui_input";
	};
}
