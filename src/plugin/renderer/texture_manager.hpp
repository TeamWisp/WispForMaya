#pragma once

// C++ standard
#include <memory>
#include <vector>

// Wisp forward declarations
namespace wr
{
	struct TextureHandle;

	class TexturePool;
}

namespace wmr
{
	class TextureManager
	{
	public:
		TextureManager();
		~TextureManager() = default;

		//! Get a texture handle to the fall-back texture
		const wr::TextureHandle& GetDefaultTexture() const noexcept;

	private:
		//! Holds all texture handles of the texture manager
		/*! Index 0 is always available because the default fall-back texture is stored at that location. */
		std::vector<wr::TextureHandle> m_texture_container;

		//! Wisp texture pool
		std::unique_ptr<wr::TexturePool> m_texture_pool;
	};
}