#include "texture_manager.hpp"

// Wisp plug-in
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"
#include "settings.hpp"

// Wisp rendering framework
#include "wisp.hpp"
#include "d3d12/d3d12_renderer.hpp"

namespace wmr
{
	TextureManager::TextureManager()
	{
		// Create a texture pool using the D3D12 Wisp renderer
		m_texture_pool.reset(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetD3D12Renderer().CreateTexturePool().get());

		// The default texture needs to be loaded at all times
		m_texture_container.push_back(m_texture_pool->Load("./resources/textures/default_texture.png", false, false));
	}

	const wr::TextureHandle& TextureManager::GetDefaultTexture() const noexcept
	{
		// This is safe because this class ensures that there is always a fall-back texture on index 0
		return m_texture_container[0];
	}
}