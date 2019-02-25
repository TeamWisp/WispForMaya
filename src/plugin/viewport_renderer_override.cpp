#include "viewport_renderer_override.hpp"

// Wisp plug-in
#include "miscellaneous/functions.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/parsers/scene_graph_parser.hpp"
#include "render_operations/screen_render_operation.hpp"
#include "render_operations/gizmo_render_operation.hpp"
#include "renderer/renderer.hpp"

// Maya API
#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnCamera.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MImage.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MString.h>

// C++ standard
#include <algorithm>
#include <memory>
#include <sstream>

auto window = std::make_unique<wr::Window>( GetModuleHandleA( nullptr ), "D3D12 Test App", 1280, 720 );

static std::shared_ptr<wr::TexturePool> texture_pool;
static std::shared_ptr<wr::MaterialPool> material_pool;
static wr::TextureHandle loaded_skybox;
static wr::TextureHandle loaded_skybox2;

#define SCENE viknell_scene

namespace wmr
{
	static void EnsurePanelDisplayShading( const MString& destination )
	{
		M3dView view;

		if( destination.length() &&
			M3dView::getM3dViewFromModelPanel( destination, view ) == MStatus::kSuccess )
		{
			if( view.displayStyle() != M3dView::kGouraudShaded )
			{
				view.setDisplayStyle( M3dView::kGouraudShaded );
			}
		}
	}

	static void CreateScene( wr::SceneGraph* scene_graph, wr::Window* window )
	{

		//loaded_skybox2 = texture_pool->Load( "resources/materials/LA_Downtown_Afternoon_Fishing_3k.hdr", false, false );
		//loaded_skybox = texture_pool->Load( "resources/materials/skybox.dds", false, false );

		//scene_graph->m_skybox = loaded_skybox2;

		//auto skybox = scene_graph->CreateChild<wr::SkyboxNode>( nullptr, loaded_skybox );

		//// Lights
		//auto point_light_0 = scene_graph->CreateChild<wr::LightNode>( nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 5, 5 } );
		//point_light_0->SetRadius( 30.0f );
		//point_light_0->SetPosition( { -10, 0, 0 } );

		//auto point_light_1 = scene_graph->CreateChild<wr::LightNode>( nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 5, 5 } );
		//point_light_1->SetRadius( 20.0f );
		//point_light_1->SetPosition( { 0, 0, 10.0 } );

		//auto point_light_2 = scene_graph->CreateChild<wr::LightNode>( nullptr, wr::LightType::POINT, DirectX::XMVECTOR{ 5, 5, 5 } );
		//point_light_2->SetRadius( 12.0f );
		//point_light_2->SetPosition( { -0.7, 3.5, 0 } );

	}

	ViewportRendererOverride::ViewportRendererOverride(const MString& name)
		: MRenderOverride(name)
		, m_ui_name(wmr::settings::PRODUCT_NAME)
		, m_current_render_operation(-1)
	{
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();
		if( maya_renderer )
		{
			maya_renderer->registerOverride( this );
		}
		else
		{
			assert( false );
		}

		ConfigureRenderOperations();
		SetDefaultTextureState();

		CreateRenderOperations();

		m_scenegraph_parser = std::make_unique<ScenegraphParser>(*m_renderer->);
		m_scenegraph_parser->initialize(texture_pool, material_pool);

		
	}

	ViewportRendererOverride::~ViewportRendererOverride()
	{
		ReleaseTextureResources();
		// Not the Wisp renderer, but the internal Maya renderer
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if( maya_renderer )
		{
			// De-register the actual plug-in
			maya_renderer->deregisterOverride( this );
		}
	}

	void ViewportRendererOverride::ConfigureRenderOperations()
	{
		m_render_operation_names[0] = "wisp_SceneBlit";
		m_render_operation_names[1] = "wisp_UIDraw";
		m_render_operation_names[2] = "wisp_Present";
	}

	void ViewportRendererOverride::SetDefaultTextureState()
	{
		m_color_texture.texture = nullptr;
		m_color_texture_desc.setToDefault2DTexture();

		m_depth_texture.texture = nullptr;
		m_depth_texture_desc.setToDefault2DTexture();
	}

	void ViewportRendererOverride::ReleaseTextureResources() const
	{
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
			return;

		const auto maya_texture_manager = maya_renderer->getTextureManager();

		if (!maya_texture_manager)
			return;

		if (m_color_texture.texture)
			maya_texture_manager->releaseTexture(m_color_texture.texture);

		if (m_depth_texture.texture)
			maya_texture_manager->releaseTexture(m_depth_texture.texture);
	}

	void ViewportRendererOverride::CreateRenderOperations()
	{
		if (!m_render_operations[0])
		{
			m_render_operations[0] = std::make_unique<ScreenRenderOperation>(m_render_operation_names[0]);
			m_render_operations[1] = std::make_unique<GizmoRenderOperation>(m_render_operation_names[1]);
			m_render_operations[2] = std::make_unique<MHWRender::MHUDRender>();
			m_render_operations[3] = std::make_unique<MHWRender::MPresentTarget>(m_render_operation_names[2]);
		}
	}
		

	MHWRender::DrawAPI ViewportRendererOverride::supportedDrawAPIs() const
	{
		return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
	}

	MHWRender::MRenderOperation* ViewportRendererOverride::renderOperation()
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

	void ViewportRendererOverride::SynchronizeWispWithMayaViewportCamera()
	{
		M3dView maya_view;
		MStatus status = M3dView::getM3dViewFromModelPanel( wmr::settings::VIEWPORT_PANEL_NAME, maya_view );

		if( status != MStatus::kSuccess )
		{
			// Failure means no camera data for this frame, early-out!
			return;
		}

		// Model view matrix
		MMatrix mv_matrix;
		maya_view.modelViewMatrix( mv_matrix );

		MDagPath camera_dag_path;
		maya_view.getCamera( camera_dag_path );
		MFnTransform camera_transform( camera_dag_path.transform() );
		// Additional functionality
		
		MEulerRotation view_rotation;
		camera_transform.getRotation( view_rotation );
	
		m_viewport_camera->SetRotation( {  ( float )view_rotation.x,( float )view_rotation.y, ( float )view_rotation.z } );

		
		MMatrix cameraPos = camera_dag_path.inclusiveMatrix();
		MVector eye = MVector( static_cast<float>( cameraPos(3,0)), static_cast< float >( cameraPos( 3, 1 ) ), static_cast< float >( cameraPos( 3, 2 ) ) );
		m_viewport_camera->SetPosition( { ( float )-eye.x, ( float )-eye.y, ( float )-eye.z } );

		
		MFnCamera camera_functions( camera_dag_path );
		m_viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
		m_viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();
		

		unsigned int target_width = 0;
		unsigned int target_height = 0;
		MHWRender::MRenderer::theRenderer()->outputTargetSize( target_width, target_height );

		double hfov = camera_functions.horizontalFieldOfView();
		double vfov = camera_functions.verticalFieldOfView();
		m_viewport_camera->SetAspectRatio( 1.0f );

		m_viewport_camera->SetFov( AI_RAD_TO_DEG(hfov) );


		m_viewport_camera->SetFov( AI_RAD_TO_DEG( camera_functions.horizontalFieldOfView()) );
	}

	Renderer & ViewportRendererOverride::GetRenderer() const
	{
		return *m_renderer.get();
	}

	MStatus ViewportRendererOverride::setup(const MString& destination)
	{
		SynchronizeWispWithMayaViewportCamera();

		auto* const maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
		{
			assert( false ); 
			return MStatus::kFailure;
		}

		auto* const maya_texture_manager = maya_renderer->getTextureManager();

		if (!maya_texture_manager)
		{
			assert( false );
			return MStatus::kFailure;
		}

		if ( AreAllRenderOperationsSetCorrectly() )
		{
			assert( false );
			return MStatus::kFailure;
		}

		// Update textures used for scene blit
		if (!UpdateTextures(maya_renderer, maya_texture_manager, m_renderer->GetRenderResult()))
		{
			assert( false );
			return MStatus::kFailure;
		}

		// Force the panel display style to smooth shaded if it is not already
		// this ensures that viewport selection behavior works as if shaded
		EnsurePanelDisplayShading(destination);

		return MStatus::kSuccess;
	}

	bool ViewportRendererOverride::AreAllRenderOperationsSetCorrectly() const
	{
		return (!m_render_operations[0] ||
				!m_render_operations[1] ||
				!m_render_operations[2] ||
				!m_render_operations[3]);
	}

	// TODO: REFACTOR
	void ViewportRendererOverride::UpdateTextureData(MHWRender::MTextureAssignment& texture_to_update, WispBufferType type, const wr::CPUTexture& cpu_texture, MHWRender::MTextureManager* texture_manager)
	{
		unsigned int buffer_width = cpu_texture.m_buffer_width;
		unsigned int buffer_height = cpu_texture.m_buffer_height;
		unsigned int buffer_bytes_per_pixel = cpu_texture.m_bytes_per_pixel;

		// Allocate memory to store the Wisp texture data
		auto* wisp_data = new unsigned char[buffer_width * buffer_height * buffer_bytes_per_pixel];
		memcpy(wisp_data, cpu_texture.m_data, sizeof(unsigned char) * buffer_width * buffer_height * buffer_bytes_per_pixel);

		if (!texture_to_update.texture)
		{
			switch (type)
			{
			case WispBufferType::COLOR:
				m_color_texture_desc.fWidth = buffer_width;
				m_color_texture_desc.fHeight = buffer_height;
				m_color_texture_desc.fDepth = 1;
				m_color_texture_desc.fBytesPerRow = buffer_bytes_per_pixel * buffer_width;
				m_color_texture_desc.fBytesPerSlice = m_color_texture_desc.fBytesPerRow * buffer_height;
				m_color_texture_desc.fTextureType = MHWRender::kImage2D;

				// Acquire a new texture
				m_color_texture.texture = texture_manager->acquireTexture("", m_color_texture_desc, wisp_data);

				if (m_color_texture.texture)
					m_color_texture.texture->textureDescription(m_color_texture_desc);
				break;

			case WispBufferType::DEPTH:
				m_depth_texture_desc.fWidth = buffer_width;
				m_depth_texture_desc.fHeight = buffer_height;
				m_depth_texture_desc.fDepth = 1;
				m_depth_texture_desc.fBytesPerRow = buffer_bytes_per_pixel * buffer_width;
				m_depth_texture_desc.fBytesPerSlice = m_color_texture_desc.fBytesPerRow * buffer_height;
				m_depth_texture_desc.fTextureType = MHWRender::kDepthTexture;

				// Acquire a new texture
				m_depth_texture.texture = texture_manager->acquireDepthTexture("", reinterpret_cast<float*>(wisp_data), buffer_width, buffer_height, false, nullptr);

				if (m_depth_texture.texture)
					m_depth_texture.texture->textureDescription(m_depth_texture_desc);
				break;

			default:
				break;
			}
		}
		else
		{
			switch (type)
			{
			case wmr::WispBufferType::COLOR:
				m_color_texture.texture->update(wisp_data, false);
				break;
			
			case wmr::WispBufferType::DEPTH:
				m_depth_texture.texture->update(wisp_data, false);
				break;

			default:
				break;
			}
		}

		delete[] wisp_data;
	}

	MStatus ViewportRendererOverride::cleanup()
	{
		m_current_render_operation = -1;
		return MStatus::kSuccess;
	}

	MString ViewportRendererOverride::uiName() const
	{
		return m_ui_name;
	}

	bool ViewportRendererOverride::startOperationIterator()
	{
		m_current_render_operation = 0;
		return true;
	}

	bool ViewportRendererOverride::nextRenderOperation()
	{
		++m_current_render_operation;

		if (m_current_render_operation < 4)
		{
			return true;
		}

		return false;
	}

	bool ViewportRendererOverride::UpdateTextures(MHWRender::MRenderer* maya_renderer, MHWRender::MTextureManager* texture_manager, const wr::CPUTextures& cpu_textures)
	{
		if (!maya_renderer || !texture_manager)
			return false;

		// Early exit, no texture data from Wisp available just yet
		if (cpu_textures.pixel_data == std::nullopt ||
			cpu_textures.depth_data == std::nullopt)
			return true;

		// Get current output size.
		unsigned int target_width = 0;
		unsigned int target_height = 0;
		maya_renderer->outputTargetSize(target_width, target_height);

		bool aquire_new_texture = false;
		bool force_reload = true;

		// If a resize occurred, or we haven't allocated any texture yet,
		// then create new textures which match the output size. 
		// Release any existing textures.
		//
		if (force_reload || !m_color_texture.texture ||
			(m_color_texture_desc.fWidth != target_width || m_color_texture_desc.fHeight != target_height) ||
			(m_depth_texture_desc.fWidth != target_width || m_depth_texture_desc.fHeight != target_height))
		{
			if (m_color_texture.texture)
			{
				texture_manager->releaseTexture(m_color_texture.texture);
				m_color_texture.texture = nullptr;
			}

			if (m_depth_texture.texture)
			{
				texture_manager->releaseTexture(m_depth_texture.texture);
				m_depth_texture.texture = nullptr;
			}
			
			aquire_new_texture = true;
			force_reload = false;
		}

		UpdateTextureData(m_color_texture, WispBufferType::COLOR, cpu_textures.pixel_data.value(), texture_manager);
		UpdateTextureData(m_depth_texture, WispBufferType::DEPTH, cpu_textures.depth_data.value(), texture_manager);

		// Update the textures used for the blit operation.
		if (aquire_new_texture)
		{
			auto* custom_blit = (ScreenRenderOperation*)m_render_operations[0].get();
			custom_blit->SetColorTexture(m_color_texture);
			custom_blit->SetDepthTexture(m_depth_texture);
		}

		return (m_color_texture.texture && m_depth_texture.texture);
	}
}
