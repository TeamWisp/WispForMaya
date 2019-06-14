#pragma once

// Wisp rendering framework
#include "structs.hpp"

// C++ standard
#include <memory>
#include <unordered_map>

#include <maya/MGlobal.h>

// Wisp forward declarations
namespace wr
{
	class TexturePool;
}

namespace wmr
{
	class Renderer;

	class TextureManager
	{
	public:
		TextureManager(Renderer *renderer);
		~TextureManager() = default;

		//! Initialization
		void Initialize() noexcept;

		//! Deallocate the texture pool
		void Destroy() noexcept;

		//! Create a new texture
		const std::shared_ptr<wr::TextureHandle> CreateTexture(const char* path) noexcept;

		//! Get a texture handle to the fall-back texture
		const wr::TextureHandle GetDefaultSkybox() const noexcept;

		//! Get a texture handle by name
		const std::shared_ptr<wr::TextureHandle> GetTexture(const wr::TextureHandle& texture_handle) noexcept;

		const std::shared_ptr<wr::TexturePool> GetTexturePool() noexcept;

		//! Indicate that a texture is no longer in use by a mesh
		/*! Internally, this will check the reference counter and once
		 *  no other objects are using the texture handle anymore, this
		 *  function will deallocate the memory automatically.
		 *  
		 *  \returns : Whether application actually deallocates the memory in Wisp. */
		bool MarkTextureUnused(const wr::TextureHandle& texture_handle) noexcept;

	private:
		//! Holds all texture handles of the texture manager
		// Texture manager keeps refs and automatically gets rid of the texture once the ref count equals 1
		std::unordered_map<size_t, std::shared_ptr<wr::TextureHandle>> m_texture_container;

		//! Default texture that can always be used (our Wisp skybox texture)
		wr::TextureHandle m_default_texture;

		// Reference to the renderer that will allow us to get access to the internal frame index
		Renderer& m_renderer;

		//! Wisp texture pool
		std::shared_ptr<wr::TexturePool> m_texture_pool;
	};
}