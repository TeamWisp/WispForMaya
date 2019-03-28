#pragma once

#include <vector>

// Maya API
#include <maya/MApiNamespace.h>

// C++ standard
#include <optional>

namespace wr
{
	class MaterialHandle;
	class Material;
}

namespace wmr
{
	class Renderer;
	class TextureManager;
	class MaterialManager;

	namespace detail
	{
		enum class SurfaceShaderType
		{
			UNSUPPORTED = -1,

			LAMBERT,
			PHONG,
			ARNOLD_STANDARD_SURFACE_SHADER
		};

		struct ArnoldStandardSurfaceShaderData
		{
			// Plug names to retrieve values
			static const constexpr char* diffuse_color_plug_name		= "baseColor";
			static const constexpr char* diffuse_roughness_plug_name	= "diffuseRoughness";
			static const constexpr char* metalness_plug_name			= "metalness";
			static const constexpr char* specular_color_plug_name		= "specularColor";
			static const constexpr char* specular_roughness_plug_name	= "specularRoughness";

			// Flags
			bool using_diffuse_color_value		= true;
			bool using_diffuse_roughness_value	= true;
			bool using_metalness_value			= true;
			bool using_specular_color_value		= true;
			bool using_specular_roughness_value	= true;

			// Values
			float diffuse_color[3]		= { 0.0f, 0.0f, 0.0f };
			float diffuse_roughness		= 0.0f;
			float metalness				= 0.0f;
			float specular_color[3]		= { 0.0f, 0.0f, 0.0f };
			float specular_roughness	= 0.0f;

			// Files
			const char* bump_map_texture_path			= "";
			const char* diffuse_color_texture_path		= "";
			const char* diffuse_roughness_texture_path	= "";
			const char* metalness_texture_path			= "";
			const char* specular_color_texture_path		= "";
			const char* specular_roughness_texture_path = "";
		};
	}

	namespace MayaMaterialProps
	{
		static const constexpr char* arnold_standard_shader_name = "aiStandardSurface";
		static const constexpr char* maya_phong_shader_name = "phong";
		static const constexpr char* maya_lambert_shader_name = "lambert";

		static const constexpr char * surface_shader = "surfaceShader";
					 
		static const constexpr char * plug_color = "color";
		static const constexpr char * plug_reflectivity = "reflectivity";
		static const constexpr char * plug_file_texture_name = "fileTextureName";
					 
		static const constexpr char * plug_color_r = "R";
		static const constexpr char * plug_color_g = "G";
		static const constexpr char * plug_color_b = "B";
		static const constexpr char * plug_color_a = "A";
	};

	class MaterialParser
	{
	public:
		MaterialParser();
		~MaterialParser() = default;

		void Parse(const MFnMesh& mesh);
		const std::optional<MObject> GetMeshObjectFromMaterial(MObject & object);
		void HandleLambertChange(MFnDependencyNode &fn, MPlug & plug, MString & plug_name, wr::Material & material);
		void HandlePhongChange(MFnDependencyNode &fn, MPlug & plug, MString & plug_name, wr::Material & material);
		const Renderer & GetRenderer();

	private:
		void ParseSingleSurfaceShader(const MObject& node, const MFnMesh& mesh, MaterialManager& material_manager, TextureManager& texture_manager);
		const detail::SurfaceShaderType GetShaderType(const MObject& node);
		const std::optional<MString> GetPlugTexture(MPlug& plug);
		const MPlug GetPlugByName(const MObject& node, MString name);
		const std::optional<MPlug> GetSurfaceShader(const MObject& node);

		void ConfigureWispMaterial(const detail::ArnoldStandardSurfaceShaderData& data, wr::Material* material, TextureManager& texture_manager) const;

		detail::ArnoldStandardSurfaceShaderData ParseArnoldStandardSurfaceShaderData(const MObject& plug);

		// Material parsing
		MColor GetColor(MFnDependencyNode & fn, MString & plug_name);

	private:
		// std::pair
		//    first: MObject, connected lambert plug
		//    second: MObject, MFnMesh.object()
		std::vector<std::pair<MObject, MObject>> mesh_material_relations;

		Renderer& m_renderer;
	};
}
