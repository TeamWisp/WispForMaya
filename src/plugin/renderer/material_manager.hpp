#pragma once

#include "structs.hpp"

#include <memory>


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

		wr::MaterialHandle* GetDefaultMaterial() noexcept;
	private:
		// TODO added render_system..
		
		wr::MaterialHandle m_default_material_handle;
		std::shared_ptr<wr::MaterialPool> m_material_pool;
	};

}