#include "texture_manager.hpp"

// Wisp plug-in
#include "miscellaneous/functions.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "renderer.hpp"
#include "settings.hpp"

// Wisp rendering framework
#include "d3d12/d3d12_renderer.hpp"
#include "wisp.hpp"

// C++ standard
#include <algorithm>

namespace wmr
{
	TextureManager::TextureManager()
		: m_renderer(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer())
	{}
	
	void TextureManager::Initialize() noexcept
	{
		LOG("Attempting to get a reference to the texture pool via the renderer.");
		
		// Create a texture pool using the D3D12 Wisp renderer
		m_texture_pool = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetD3D12Renderer().CreateTexturePool();

		LOG("Attempting to load the hard-coded skybox.");
		
		// The default texture needs to be loaded at all times
		m_default_texture = m_texture_pool->LoadFromFile("./resources/textures/wisp_default_skybox.png", false, false);
	}

	void TextureManager::Destroy() noexcept
	{
		m_texture_pool.reset();
	}

	const std::shared_ptr<wr::TextureHandle> TextureManager::CreateTexture(const char* path) noexcept
	{
		auto hash = func::HashCString(path);

		// Does the texture exist?
		auto it = std::find_if(m_texture_container.begin(), m_texture_container.end(), [&hash](const std::unordered_map<size_t, std::shared_ptr<wr::TextureHandle>>::value_type& vt) {
			return (vt.first == hash);
		});

		if (it == m_texture_container.end())
		{
			wr::TextureHandle texture_handle;
			// Texture does not exist yet
			texture_handle = m_texture_pool->LoadFromFile(path, false, false);
			// Return an invalid shared_ptr if the texture couldn't be loaded
			if (texture_handle.m_pool == (wr::TextureHandle()).m_pool) {
				return std::shared_ptr<wr::TextureHandle>(nullptr);
			}
			m_texture_container[hash] = std::make_shared<wr::TextureHandle>(texture_handle);
		}

		return m_texture_container[hash];
	}

	const wr::TextureHandle TextureManager::GetDefaultSkybox() const noexcept
	{
		return m_default_texture;
	}

	const std::shared_ptr<wr::TextureHandle> TextureManager::GetTexture(const char* identifier) noexcept
	{
		auto hash = func::HashCString(identifier);

		// Does the texture exist?
		auto it = std::find_if(m_texture_container.begin(), m_texture_container.end(), [&hash](const std::unordered_map<size_t, std::shared_ptr<wr::TextureHandle>>::value_type& vt) {
			return (vt.first == hash);
		});

		if (it == m_texture_container.end())
		{
			// Failed to find a valid texture
			return nullptr;
		}

		// Use a known texture
		return m_texture_container[hash];
	}

	const std::shared_ptr<wr::TexturePool> TextureManager::GetTexturePool() noexcept
	{
		return m_texture_pool;
	}

	bool TextureManager::MarkTextureUnused(const char* identifier) noexcept
	{
		auto hash = func::HashCString(identifier);

		// Does the texture exist?
		auto it = std::find_if(m_texture_container.begin(), m_texture_container.end(), [&hash](const std::unordered_map<size_t, std::shared_ptr<wr::TextureHandle>>::value_type& vt) {
			return (vt.first == hash);
		});

		if (it == m_texture_container.end())
		{
			// Texture does not even exist!
			return true;
		}

		//// Find the current number of objects that use this texture
		auto ref_count = m_texture_container[hash].use_count();

		if (ref_count == 1)
		{
			// Only reference left to this texture is the one that's in the unordered_map,
			// so the texture can be deleted.
			m_texture_pool->MarkForUnload(*m_texture_container[hash], m_renderer.GetFrameIndex());
			m_texture_container.erase(hash);
			
			// Removed the texture from the texture pool
			return true;
		}

		// Did not perform any texture deallocation
		return false;
	}
}
