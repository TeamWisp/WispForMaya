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

	namespace detail
	{
		enum class SurfaceShaderType
		{
			UNSUPPORTED = -1,

			LAMBERT,
			PHONG,
		};
	}

	namespace MayaMaterialProps
	{
		const char const * surface_shader = "surfaceShader";
		
		const char const * plug_color = "color";
		const char const * plug_reflectivity = "reflectivity";
		const char const * plug_file_texture_name = "fileTextureName";

		const char const * plug_color_r = "R";
		const char const * plug_color_g = "G";
		const char const * plug_color_b = "B";
		const char const * plug_color_a = "A";
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
		const detail::SurfaceShaderType GetShaderType(const MObject& node);
		const std::optional<MString> GetPlugTexture(MPlug& plug);
		const MPlug GetPlugByName(const MObject& node, MString name);
		const std::optional<MPlug> GetSurfaceShader(const MObject& node);

		// Material parsing
		MColor GetColor(MFnDependencyNode & fn, MString & plug_name);

		// std::pair
		//    first: MObject, connected lambert plug
		//    second: MObject, MFnMesh.object()
		std::vector<std::pair<MObject, MObject>> mesh_material_relations;

		Renderer& m_renderer;
	};
}