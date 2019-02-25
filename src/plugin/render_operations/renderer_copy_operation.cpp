#include "renderer_copy_operation.hpp"

// Wisp plug-in
#include "miscellaneous/settings.hpp"
#include "plugin/render_operations/screen_render_operation.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Maya API
#include <maya/MViewport2Renderer.h>

namespace wmr
{
	RendererCopyOperation::RendererCopyOperation(const MString& name, ScreenRenderOperation& blit_operation)
		: MHWRender::MUserRenderOperation(name)
		, m_renderer(dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer())
		, m_blit_operation(blit_operation)
	{
		SetDefaultTextureState();
	}

	RendererCopyOperation::~RendererCopyOperation()
	{
		ReleaseTextures();
	}

	void RendererCopyOperation::SetDefaultTextureState() noexcept
	{
		m_color_texture.texture = nullptr;
		m_color_texture_desc.setToDefault2DTexture();

		m_depth_texture.texture = nullptr;
		m_depth_texture_desc.setToDefault2DTexture();
	}

	void RendererCopyOperation::ReleaseTextures() noexcept
	{
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
			return;

		const auto maya_texture_manager = maya_renderer->getTextureManager();

		if (!maya_texture_manager)
			return;

		if (m_color_texture.texture)
		{
			maya_texture_manager->releaseTexture(m_color_texture.texture);
			m_color_texture.texture = nullptr;
		}

		if (m_depth_texture.texture)
		{
			maya_texture_manager->releaseTexture(m_depth_texture.texture);
			m_depth_texture.texture = nullptr;
		}
	}

	void RendererCopyOperation::CreateColorTextureOfSize(const wr::CPUTexture& cpu_data, bool& created_new_texture)
	{
		if (!m_color_texture.texture)
		{
			m_color_texture_desc.fWidth = cpu_data.m_buffer_width;
			m_color_texture_desc.fHeight = cpu_data.m_buffer_height;
			m_color_texture_desc.fDepth = 1;
			m_color_texture_desc.fBytesPerRow = cpu_data.m_bytes_per_pixel * cpu_data.m_buffer_width;
			m_color_texture_desc.fBytesPerSlice = (cpu_data.m_bytes_per_pixel * cpu_data.m_buffer_width) * cpu_data.m_buffer_height;
			m_color_texture_desc.fTextureType = MHWRender::kImage2D;

			m_color_texture.texture = MHWRender::MRenderer::theRenderer()->getTextureManager()->acquireTexture("", m_color_texture_desc, cpu_data.m_data);
			m_color_texture.texture->textureDescription(m_color_texture_desc);

			created_new_texture = true;
		}
		else
		{
			m_color_texture.texture->update(cpu_data.m_data, false);

			created_new_texture = false;
		}
	}

	void RendererCopyOperation::CreateDepthTextureOfSize(const wr::CPUTexture& cpu_data, bool& created_new_texture)
	{
		if (!m_depth_texture.texture)
		{
			m_depth_texture_desc.fWidth = cpu_data.m_buffer_width;
			m_depth_texture_desc.fHeight = cpu_data.m_buffer_height;
			m_depth_texture_desc.fDepth = 1;
			m_depth_texture_desc.fBytesPerRow = cpu_data.m_bytes_per_pixel * cpu_data.m_buffer_width;
			m_depth_texture_desc.fBytesPerSlice = (cpu_data.m_bytes_per_pixel * cpu_data.m_buffer_width) * cpu_data.m_buffer_height;
			m_depth_texture_desc.fTextureType = MHWRender::kDepthTexture;

			m_depth_texture.texture = MHWRender::MRenderer::theRenderer()->getTextureManager()->acquireDepthTexture("", cpu_data.m_data, cpu_data.m_buffer_width, cpu_data.m_buffer_height, false, nullptr);
			m_depth_texture.texture->textureDescription(m_depth_texture_desc);
		
			created_new_texture = true;
		}
		else
		{
			m_depth_texture.texture->update(cpu_data.m_data, false);

			created_new_texture = false;
		}
	}

	const MCameraOverride* RendererCopyOperation::cameraOverride()
	{
		// Not using a camera override
		return nullptr;
	}

	MStatus RendererCopyOperation::execute(const MDrawContext& draw_context)
	{
		auto maya_renderer = MHWRender::MRenderer::theRenderer();

		// Failed to retrieve the Maya renderer, cannot continue!
		if (!maya_renderer)
			return;

		auto maya_texture_manager = maya_renderer->getTextureManager();

		// Failed to retrieve the Maya texture manager, cannot continue!
		if (!maya_texture_manager)
			return;

		// Get the output of the Wisp renderer for this frame
		auto wisp_renderer_output = m_renderer.GetRenderResult();

		// If the renderer failed to give us any meaningful data, do nothing
		if (!wisp_renderer_output.depth_data.has_value() ||
			!wisp_renderer_output.pixel_data.has_value())
		{
			return;
		}

		// Retrieve the width and height of the Wisp output
		// Since Wisp has textures of the same size, there is no need to do this for the depth buffer as well, as it
		// is perfectly safe to assume that it has the exact same size as the color buffer.
		auto wisp_output_width = wisp_renderer_output.pixel_data->m_buffer_width;
		auto wisp_output_height = wisp_renderer_output.pixel_data->m_buffer_height;

		// All textures have to exist
		bool textures_exist = (m_color_texture.texture && m_depth_texture.texture);

		// Since we can assume the plug-in already made Wisp resize to the correct viewport dimensions at this point,
		// we only have to make sure that the current Maya texture matches the Wisp texture size...
		bool texture_size_matches = (m_color_texture_desc.fWidth == wisp_output_width	&&
									 m_depth_texture_desc.fWidth == wisp_output_width	&&
									 m_color_texture_desc.fHeight == wisp_output_height	&&
									 m_depth_texture_desc.fHeight == wisp_output_height);

		// Flag indicating whether a new color / depth buffer has to be created
		bool requires_new_textures = false;

		// The texture sizes do not match up, release the current textures and ask for new ones
		if (!texture_size_matches || !textures_exist)
		{
			ReleaseTextures();
		}

		// If this flag gets set, it means the blit operation needs to update its handles to the color and depth textures
		bool new_textures_available = false;

		// Create a new color and depth texture with the exact same dimensions as the Wisp output textures if there old
		// texture has been released. If there is a texture already, just update the existing data.
		CreateColorTextureOfSize(wisp_renderer_output.pixel_data.value(), new_textures_available);
		CreateDepthTextureOfSize(wisp_renderer_output.depth_data.value(), new_textures_available);

		if (new_textures_available)
		{
			m_blit_operation.SetColorTexture(m_color_texture);
			m_blit_operation.SetDepthTexture(m_depth_texture);
		}

		return MStatus::kSuccess;
	}

	bool RendererCopyOperation::hasUIDrawables() const
	{
		// Not using any UI drawable
		return false;
	}

	bool RendererCopyOperation::requiresLightData() const
	{
		// This operation does not require any light data
		return false;
	}
}
