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

namespace wmr
{
	struct LambertShaderData
	{
		// Plug names to retrieve values
		static const constexpr char* diffuse_color_plug_name = "color";
		static const constexpr char* bump_map_plug_name = "normalCamera";

		// Flags
		bool using_diffuse_color_value = true;
		bool using_bump_map_value = true;

		// Values
		float diffuse_color[3] = {0.0f, 0.0f, 0.0f};

		// Files
		const char* diffuse_color_texture_path = "";
		const char* bump_map_texture_path = "";
	};

	struct PhongShaderData
	{
		// Plug names to retrieve values
		static const constexpr char* diffuse_color_plug_name = "color";
		static const constexpr char* bump_map_plug_name = "normalCamera";

		// Flags
		bool using_diffuse_color_value = true;
		bool using_bump_map_value = true;

		// Values
		float diffuse_color[3] = {0.0f, 0.0f, 0.0f};

		// Files
		const char* diffuse_color_texture_path = "";
		const char* bump_map_texture_path = "";
	};

	struct ArnoldStandardSurfaceShaderData
	{
		// Plug names to retrieve values
		static const constexpr char* diffuse_color_plug_name = "baseColor";
		static const constexpr char* metalness_plug_name = "metalness";
		static const constexpr char* emission_plug_name = "emission";
		static const constexpr char* emission_color_plug_name = "emissionColor";
		static const constexpr char* roughness_plug_name = "specularRoughness";
		static const constexpr char* bump_map_plug_name = "normalCamera";

		// Flags
		bool using_diffuse_color_value = true;
		bool using_metalness_value = true;
		bool using_emission_value = true;
		bool using_emission_color_value = true;
		bool using_roughness_value = true;

		// Values
		float diffuse_color[3] = {0.0f, 0.0f, 0.0f};
		float metalness = 0.0f;
		float emission = 0.0f;
		float roughness = 0.0f;

		// Files
		const char* bump_map_texture_path = "";
		const char* diffuse_color_texture_path = "";
		const char* metalness_texture_path = "";
		const char* emission_color_texture_path = "";
		const char* roughness_texture_path = "";
	};
}