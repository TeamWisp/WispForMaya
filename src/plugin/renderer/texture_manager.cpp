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
	void TextureManager::Initialize() noexcept
	{
		// Create a texture pool using the D3D12 Wisp renderer
		m_texture_pool = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetD3D12Renderer().CreateTexturePool();

		// The default texture needs to be loaded at all times
		m_default_texture = m_texture_pool->Load("./resources/textures/default_texture.png", false, false);
	}

	const wr::TextureHandle TextureManager::GetDefaultTexture() const noexcept
	{
		return m_default_texture;
	}
}