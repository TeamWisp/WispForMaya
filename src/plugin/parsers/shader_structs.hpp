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
		static const constexpr char* diffuse_roughness_plug_name = "diffuseRoughness";
		static const constexpr char* metalness_plug_name = "metalness";
		static const constexpr char* specular_color_plug_name = "specularColor";
		static const constexpr char* specular_roughness_plug_name = "specularRoughness";
		static const constexpr char* bump_map_plug_name = "normalCamera";

		// Flags
		bool using_diffuse_color_value = true;
		bool using_diffuse_roughness_value = true;
		bool using_metalness_value = true;
		bool using_specular_color_value = true;
		bool using_specular_roughness_value = true;

		// Values
		float diffuse_color[3] = {0.0f, 0.0f, 0.0f};
		float diffuse_roughness = 0.0f;
		float metalness = 0.0f;
		float specular_color[3] = {0.0f, 0.0f, 0.0f};
		float specular_roughness = 0.0f;

		// Files
		const char* bump_map_texture_path = "";
		const char* diffuse_color_texture_path = "";
		const char* diffuse_roughness_texture_path = "";
		const char* metalness_texture_path = "";
		const char* specular_color_texture_path = "";
		const char* specular_roughness_texture_path = "";
	};
}