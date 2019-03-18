#pragma once

#include "structs.hpp"
#include "d3d12/d3d12_material_pool.hpp"

// Maya API
#include <maya/MApiNamespace.h>

#include <memory>
#include <vector>

namespace wr
{
	class MaterialPool;
}

namespace wmr
{
	class MaterialManager
	{
	public:
		MaterialManager();
		~MaterialManager();

		void Initialize();

		wr::MaterialHandle GetDefaultMaterial() noexcept;
		wr::MaterialHandle CreateMaterial(MObject& object);
		wr::MaterialHandle DoesExist(MObject& object);
	private:
		// TODO added render_system..

		std::vector<std::pair<MObject, wr::MaterialHandle>> m_object_material_vector;
		
		wr::MaterialHandle m_default_material_handle;
		std::shared_ptr<wr::MaterialPool> m_material_pool;
	};

}