#pragma once

// Maya API
#include <maya/MApiNamespace.h>

namespace wmr
{
	namespace detail
	{
		enum class SurfaceShaderType
		{
			UNSUPPORTED = -1,

			LAMBERT,
			PHONG,
			BLINN_PHONG
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
		const detail::SurfaceShaderType GetShaderType();
	};
}