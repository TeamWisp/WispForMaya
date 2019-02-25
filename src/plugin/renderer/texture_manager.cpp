#include "texture_manager.hpp"

// Wisp rendering framework
#include "wisp.hpp"

namespace wmr
{
	TextureManager::TextureManager()
	{
		// The default texture needs to be loaded at all times
		m_texture_container.push_back(m_texture_pool->Load("./resources/textures/default_texture.png", false, false));
	}

	const wr::TextureHandle& TextureManager::GetDefaultTexture() const noexcept
	{
		// This is safe because this class ensures that there is always a fall-back texture on index 0
		return m_texture_container[0];
	}
}