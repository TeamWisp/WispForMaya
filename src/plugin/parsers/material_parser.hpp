#pragma once

#include <vector>

// Wisp Maya Renderer
#include "shader_structs.hpp"

// Maya API
#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
#include <maya/MMessage.h>

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
			ARNOLD_STANDARD_SURFACE_SHADER
		};
	}

	namespace MayaMaterialProps
	{
		static const constexpr char* arnold_standard_shader_name = "aiStandardSurface";
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

		void InitialMaterialBuild(MPlug & surface_shader, detail::SurfaceShaderType shader_type, wr::MaterialHandle material_handle, MaterialManager & material_manager, TextureManager & texture_manager);

		void OnMeshAdded(MFnMesh& mesh);
		void OnCreateSurfaceShader(MPlug & surface_shader);
		void OnRemoveSurfaceShader(MPlug & surface_shader);

		void ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine);
		void DisconnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine);
		void ConnectMeshToShadingEngine(MObject & mesh, MObject & shading_engine);
		void DisconnectMeshFromShadingEngine(MObject & mesh, MObject & shading_engine);

		void HandleLambertChange(MFnDependencyNode &fn, MPlug & plug, MString & plug_name, wr::Material & material);
		void HandleArnoldChange(MFnDependencyNode &fn, MPlug & plug, MString & plug_name, wr::Material & material);

		const Renderer & GetRenderer();
		const detail::SurfaceShaderType GetShaderType(const MObject& node);

		struct ShaderDirtyData
		{
			MCallbackId callback_id;				// When removing callbacks, use this id to find what callback must be deleted
			wmr::MaterialParser * material_parser;	// Pointer to the material parser
			MObject surface_shader;					// Surface shader
		};

	private:
		void SubscribeSurfaceShader(MObject & actual_surface_shader);
		void ParseShadingEngineToWispMaterial(MObject & shading_engine, MObject & fnmesh);

		const std::optional<MPlug> GetSurfaceShader(const MObject& node);
		const std::optional<MPlug> GetActualSurfaceShaderPlug(const MPlug & surface_shader_plug);

		const std::optional<MString> GetPlugTexture(MPlug& plug);
		const MPlug GetPlugByName(const MObject& node, MString name);

		void ConfigureWispMaterial(const wmr::ArnoldStandardSurfaceShaderData& data, wr::Material* material, TextureManager& texture_manager) const;
		void ConfigureWispMaterial(const wmr::LambertShaderData& data, wr::Material* material, TextureManager& texture_manager) const;

		wmr::ArnoldStandardSurfaceShaderData ParseArnoldStandardSurfaceShaderData(const MObject& plug);
		wmr::LambertShaderData ParseLambertShaderData(const MObject& plug);

		// Material parsing
		MColor GetColor(MFnDependencyNode & fn, MString & plug_name);
		std::vector<ShaderDirtyData*> shader_dirty_datas;

	private:
		Renderer& m_renderer;
	};
}
