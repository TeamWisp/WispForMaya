#pragma once

// Wisp rendering framework
#include "structs.hpp"

// C++ standard
#include <memory>
#include <vector>

// Wisp forward declarations
namespace wr
{
	class TexturePool;
}

namespace wmr
{
	class TextureManager
	{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		//! Initialization
		void Initialize() noexcept;

		//! Get a texture handle to the fall-back texture
		const wr::TextureHandle GetDefaultTexture() const noexcept;

	private:
		//! Holds all texture handles of the texture manager
		/*! Index 0 is always available because the default fall-back texture is stored at that location. */
		std::vector<wr::TextureHandle> m_texture_container;

		wr::TextureHandle m_default_texture;

		//! Wisp texture pool
		std::shared_ptr<wr::TexturePool> m_texture_pool;
	};
}