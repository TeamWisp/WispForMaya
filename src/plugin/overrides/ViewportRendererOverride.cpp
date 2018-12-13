#include "ViewportRendererOverride.hpp"
#include "QuadRendererOverride.hpp"
#include "UIOverride.hpp"
#include "plugin/renderer/RendererMain.hpp"
#include "miscellaneous/Settings.hpp"

#include <maya/MImage.h>
#include <maya/M3dView.h>

namespace wmr
{
	WispViewportRenderer::WispViewportRenderer(const MString& name)
		: MRenderOverride(name)
		, m_ui_name(wisp::settings::PRODUCT_NAME)
		, m_current_render_operation(-1)
		, m_load_images_from_disk(true)
	{
		ConfigureRenderOperations();
		SetDefaultColorTextureState();
	}

	WispViewportRenderer::~WispViewportRenderer()
	{
		ReleaseColorTextureResources();
	}

	void WispViewportRenderer::Initialize()
	{
		CreateRenderOperations();
		CreateWispRenderer();
		InitializeWispRenderer();
	}

	void WispViewportRenderer::Destroy()
	{
		m_wisp_renderer_instance->Cleanup();
	}

	void WispViewportRenderer::ConfigureRenderOperations()
	{
		m_render_operation_names[0] = "wisp_SceneBlit";
		m_render_operation_names[1] = "wisp_UIDraw";
		m_render_operation_names[2] = "wisp_Present";
	}

	void WispViewportRenderer::SetDefaultColorTextureState()
	{
		m_color_texture.texture = nullptr;
		m_color_texture_desc.setToDefault2DTexture();
	}

	void WispViewportRenderer::ReleaseColorTextureResources() const
	{
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
		{
			return;
		}

		const auto maya_texture_manager = maya_renderer->getTextureManager();

		if (!maya_texture_manager || !m_color_texture.texture)
		{
			return;
		}

		maya_texture_manager->releaseTexture(m_color_texture.texture);
	}

	void WispViewportRenderer::CreateRenderOperations()
	{
		if (!m_render_operations[0])
		{
			m_render_operations[0] = std::make_unique<WispScreenBlitter>(m_render_operation_names[0]);
			m_render_operations[1] = std::make_unique<WispUIRenderer>(m_render_operation_names[1]);
			m_render_operations[2] = std::make_unique<MHWRender::MHUDRender>();
			m_render_operations[3] = std::make_unique<MHWRender::MPresentTarget>(m_render_operation_names[2]);
		}
	}

	void WispViewportRenderer::CreateWispRenderer()
	{
		m_wisp_renderer_instance = std::make_unique<wri::RendererMain>();
	}

	void WispViewportRenderer::InitializeWispRenderer()
	{
		m_wisp_renderer_instance->Initialize();
	}

	MHWRender::DrawAPI WispViewportRenderer::supportedDrawAPIs() const
	{
		return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
	}

	MHWRender::MRenderOperation* WispViewportRenderer::renderOperation()
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

	MStatus WispViewportRenderer::setup(const MString& destination)
	{
		m_wisp_renderer_instance->Update();

		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
		{
			return MStatus::kFailure;
		}

		const auto maya_texture_manager = maya_renderer->getTextureManager();

		if (!maya_texture_manager)
		{
			return MStatus::kFailure;
		}

		const auto render_operations_set = AreAllRenderOperationsSetCorrectly();

		if (render_operations_set)
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
		EnsurePanelDisplayShading(destination);

		return MStatus::kSuccess;
	}

	bool WispViewportRenderer::AreAllRenderOperationsSetCorrectly() const
	{
		return (!m_render_operations[0] ||
			!m_render_operations[1] ||
			!m_render_operations[2] ||
			!m_render_operations[3]);
	}

	MStatus WispViewportRenderer::cleanup()
	{
		m_current_render_operation = -1;
		return MStatus::kSuccess;
	}

	MString WispViewportRenderer::uiName() const
	{
		return m_ui_name;
	}

	bool WispViewportRenderer::startOperationIterator()
	{
		m_current_render_operation = 0;
		return true;
	}

	bool WispViewportRenderer::nextRenderOperation()
	{
		++m_current_render_operation;

		if (m_current_render_operation < 4)
		{
			return true;
		}

		return false;
	}

	bool WispViewportRenderer::UpdateTextures(MHWRender::MRenderer* renderer, MHWRender::MTextureManager* texture_manager)
	{
		if (!renderer || !texture_manager)
		{
			return false;
		}

		unsigned int target_width = 0;
		unsigned int target_height = 0;

		renderer->outputTargetSize(target_width, target_height);

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
				texture_manager->releaseTexture(m_color_texture.texture);
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
			m_color_texture.texture = texture_manager->acquireTexture("", m_color_texture_desc, texture_data);

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
			auto blit = (WispScreenBlitter*)m_render_operations[0].get();
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

	void WispViewportRenderer::EnsurePanelDisplayShading(const MString& destination)
	{
		M3dView view;

		if (destination.length() &&
			M3dView::getM3dViewFromModelPanel(destination, view) == MStatus::kSuccess)
		{
			if (view.displayStyle() != M3dView::kGouraudShaded)
			{
				view.setDisplayStyle(M3dView::kGouraudShaded);
			}
		}
	}
}
