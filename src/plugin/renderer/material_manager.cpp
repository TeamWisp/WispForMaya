#include "material_manager.hpp"

#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "miscellaneous/settings.hpp"

#include "d3d12/d3d12_renderer.hpp"

#include <maya/MViewport2Renderer.h>


wmr::MaterialManager::MaterialManager()
{
}

wmr::MaterialManager::~MaterialManager()
{
}

void wmr::MaterialManager::Initialize()
{
	auto& renderer = dynamic_cast< const ViewportRendererOverride* >( MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME ) )->GetRenderer();
	m_material_pool = renderer.GetD3D12Renderer().CreateMaterialPool( 0 );

	m_default_material_handle = m_material_pool->Create();

	wr::Material* internal_material = m_material_pool->GetMaterial( m_default_material_handle.m_id );
	auto& texture_manager = renderer.GetTextureManager();

	internal_material->SetAlbedo( texture_manager.GetDefaultTexture() );
	internal_material->SetNormal( texture_manager.GetDefaultTexture() );
	internal_material->SetMetallic( texture_manager.GetDefaultTexture() );
	internal_material->SetRoughness( texture_manager.GetDefaultTexture() );
}

wr::MaterialHandle wmr::MaterialManager::GetDefaultMaterial() noexcept
{
	return m_default_material_handle;
}
