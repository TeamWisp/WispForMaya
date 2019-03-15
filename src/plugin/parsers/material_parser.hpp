#pragma once

// Maya API
#include <maya/MApiNamespace.h>

// C++ standard
#include <optional>

namespace wmr
{
	namespace detail
	{
		enum class SurfaceShaderType
		{
			UNSUPPORTED = -1,

			LAMBERT,
			PHONG,
		};
	}

	struct MaterialData
	{
	};

	class MaterialParser
	{
	public:
		MaterialParser() = default;
		~MaterialParser() = default;

		void Parse(const MFnMesh& mesh);

	private:
		const detail::SurfaceShaderType GetShaderType(const MObject& node);
		const MString GetPlugTexture(MPlug& plug);
		const MPlug GetPlugByName(const MObject& node, MString name);
		const std::optional<MPlug> GetSurfaceShader(const MObject& node);
	};
}