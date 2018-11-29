#include "ViewportRendererOverride.hpp"
#include "QuadRendererOverride.hpp"
#include "UIOverride.hpp"
#include "plugin/renderer/RendererMain.hpp"

#include <maya/MImage.h>
#include <maya/M3dView.h>

wmr::WispViewportRenderer::WispViewportRenderer(const MString& t_name)
	: MRenderOverride(t_name)
	, m_ui_name("Realtime ray-traced viewport by Team Wisp")
	, m_current_render_operation(-1)
	, m_load_images_from_disk(true)
{
	m_render_operation_names[0] = "wisp_SceneBlit";
	m_render_operation_names[1] = "wisp_UIDraw";
	m_render_operation_names[2] = "wisp_Present";

	m_color_texture.texture = nullptr;
	m_color_texture_desc.setToDefault2DTexture();

	// Entry point for the Wisp renderer
	m_wisp_renderer = std::make_unique<wri::RendererMain>();
	m_wisp_renderer->StartWispRenderer();
}

wmr::WispViewportRenderer::~WispViewportRenderer()
{
	// This will clean-up any Wisp resources
	m_wisp_renderer->StopWispRenderer();

	MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();
	MHWRender::MTextureManager* maya_texture_manager = maya_renderer ? maya_renderer->getTextureManager() : nullptr;

	// Release textures
	if (maya_texture_manager)
	{
		if (m_color_texture.texture)
		{
			maya_texture_manager->releaseTexture(m_color_texture.texture);
		}
	}
}

MHWRender::DrawAPI wmr::WispViewportRenderer::supportedDrawAPIs() const
{
	return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
}

MHWRender::MRenderOperation* wmr::WispViewportRenderer::renderOperation()
{
	if (m_current_render_operation >= 0 && m_current_render_operation < 4)
	{
		if (m_render_operations[m_current_render_operation])
		{
			return m_render_operations[m_current_render_operation].get();
		}
	}
	return nullptr;
}

MStatus wmr::WispViewportRenderer::setup(const MString& t_destination)
{
	MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();

	if (!maya_renderer)
	{
		return MStatus::kFailure;
	}

	MHWRender::MTextureManager* maya_texture_manager = maya_renderer->getTextureManager();

	if (!maya_texture_manager)
	{
		return MStatus::kFailure;
	}

	// Create a new set of operations if required
	if (!m_render_operations[0])
	{
		m_render_operations[0] = std::make_unique<wmr::WispScreenBlitter>(m_render_operation_names[0]);
		m_render_operations[1] = std::make_unique<wmr::WispUIRenderer>(m_render_operation_names[1]);
		m_render_operations[2] = std::make_unique<MHWRender::MHUDRender>();
		m_render_operations[3] = std::make_unique<MHWRender::MPresentTarget>(m_render_operation_names[2]);
	}

	if (!m_render_operations[0] ||
		!m_render_operations[1] ||
		!m_render_operations[2] ||
		!m_render_operations[3])
	{
		return MStatus::kFailure;
	}

	// Update textures used for scene blit
	if (!UpdateTextures(maya_renderer, maya_texture_manager))
	{
		return MStatus::kFailure;
	}

	// Force the panel display style to smooth shaded if it is not already
	// this ensures that viewport selection behavior works as if shaded
	M3dView view;

	if (t_destination.length() &&
		M3dView::getM3dViewFromModelPanel(t_destination, view) == MStatus::kSuccess)
	{
		if (view.displayStyle() != M3dView::kGouraudShaded)
		{
			view.setDisplayStyle(M3dView::kGouraudShaded);
		}
	}

	m_wisp_renderer->UpdateWispRenderer();

	return MStatus::kSuccess;
}

MStatus wmr::WispViewportRenderer::cleanup()
{
	m_current_render_operation = -1;
	return MStatus::kSuccess;
}

MString wmr::WispViewportRenderer::uiName() const
{
	return m_ui_name;
}

bool wmr::WispViewportRenderer::startOperationIterator()
{
	m_current_render_operation = 0;
	return true;
}

bool wmr::WispViewportRenderer::nextRenderOperation()
{
	++m_current_render_operation;
	
	if (m_current_render_operation < 4)
	{
		return true;
	}

	return false;
}

bool wmr::WispViewportRenderer::UpdateTextures(MHWRender::MRenderer* t_renderer, MHWRender::MTextureManager* t_texture_manager)
{
	if (!t_renderer || !t_texture_manager)
	{
		return false;
	}

	unsigned int target_width = 0;
	unsigned int target_height = 0;

	t_renderer->outputTargetSize(target_width, target_height);

	bool aquire_new_texture = false;
	bool force_reload = false;

	bool texture_resized = (m_color_texture_desc.fWidth != target_width ||
							m_color_texture_desc.fHeight != target_height);

	MString image_location(MString(getenv("MAYA_2018_DIR")) + MString("\\devkit\\plug-ins\\viewImageBlitOverride\\"));
	image_location += MString("renderedImage.iff");

	// If a resize occurred, or a texture has not been allocated yet, create new textures that match the output size
	// Any existing textures will be released
	if (force_reload ||
		!m_color_texture.texture ||
		texture_resized)
	{
		if (m_color_texture.texture)
		{
			t_texture_manager->releaseTexture(m_color_texture.texture);
			m_color_texture.texture = nullptr;
		}

		aquire_new_texture = true;
		force_reload = false;
	}

	if (!m_color_texture.texture)
	{
		unsigned char* texture_data = nullptr;

		MImage image;

		image.readFromFile(image_location);
		image.getSize(target_width, target_height);

		texture_data = image.pixels();

		m_color_texture_desc.fWidth = target_width;
		m_color_texture_desc.fHeight = target_height;
		m_color_texture_desc.fDepth = 1;
		m_color_texture_desc.fBytesPerRow = 4 * target_width;
		m_color_texture_desc.fBytesPerSlice = m_color_texture_desc.fBytesPerRow * target_height;

		// Acquire a new texture
		m_color_texture.texture = t_texture_manager->acquireTexture("", m_color_texture_desc, texture_data);

		if (m_color_texture.texture)
		{
			m_color_texture.texture->textureDescription(m_color_texture_desc);
		}
	}
	else
	{
		// TODO: Update the texture data here!
	}

	// Update the textures for the blit operation
	if (aquire_new_texture)
	{
		auto blit = (wmr::WispScreenBlitter*)m_render_operations[0].get();
		blit->SetColorTexture(m_color_texture);
	}

	if (m_color_texture.texture)
	{
		return true;
	}
	else
	{
		return false;
	}
}
