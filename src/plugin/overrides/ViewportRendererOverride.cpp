#include "ViewportRendererOverride.hpp"
#include "QuadRendererOverride.hpp"
#include "UIOverride.hpp"
#include "miscellaneous/Settings.hpp"
#include "miscellaneous/Functions.hpp"

#include <memory>
#include <algorithm>
#include "wisp.hpp"
#include "renderer.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"
#include "render_tasks/d3d12_deferred_readback.hpp"
#include "scene_graph/camera_node.hpp"
#include "scene_graph/scene_graph.hpp"

#include "../demo/engine_interface.hpp"
#include "../demo/scene_viknell.hpp"
#include "../demo/resources.hpp"
#include "../demo/scene_cubes.hpp"

#include <maya/MString.h>
#include <maya/M3dView.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MImage.h>
#include <maya/M3dView.h>


#include <sstream>
#include <maya/MGlobal.h>


auto window = std::make_unique<wr::Window>( GetModuleHandleA( nullptr ), "D3D12 Test App", 1280, 720 );
const bool load_images_from_disk = true;

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

	ViewportRenderer::ViewportRenderer(const MString& name)
		: MRenderOverride(name)
		, m_ui_name(wisp::settings::PRODUCT_NAME)
		, m_current_render_operation(-1)
	{
		ConfigureRenderOperations();
		SetDefaultColorTextureState();
	}

	ViewportRenderer::~ViewportRenderer()
	{
		ReleaseColorTextureResources();
	}

	void ViewportRenderer::Initialize()
	{
		CreateRenderOperations();
		InitializeWispRenderer();
	}

	void ViewportRenderer::Destroy()
	{
		m_render_system->WaitForAllPreviousWork();
		m_framegraph->Destroy();
		m_render_system.reset();
		m_framegraph.reset();
	}

	void ViewportRenderer::ConfigureRenderOperations()
	{
		m_render_operation_names[0] = "wisp_SceneBlit";
		m_render_operation_names[1] = "wisp_UIDraw";
		m_render_operation_names[2] = "wisp_Present";
	}

	void ViewportRenderer::SetDefaultColorTextureState()
	{
		m_color_texture.texture = nullptr;
		m_color_texture_desc.setToDefault2DTexture();
	}

	void ViewportRenderer::ReleaseColorTextureResources() const
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

	void ViewportRenderer::CreateRenderOperations()
	{
		if (!m_render_operations[0])
		{
			m_render_operations[0] = std::make_unique<WispScreenBlitter>(m_render_operation_names[0]);
			m_render_operations[1] = std::make_unique<WispUIRenderer>(m_render_operation_names[1]);
			m_render_operations[2] = std::make_unique<MHWRender::MHUDRender>();
			m_render_operations[3] = std::make_unique<MHWRender::MPresentTarget>(m_render_operation_names[2]);
		}
	}

	void ViewportRenderer::InitializeWispRenderer()
	{
		util::log_callback::impl = [ & ]( std::string const & str )
		{
			engine::debug_console.AddLog( str.c_str() );
		};

		m_render_system = std::make_unique<wr::D3D12RenderSystem>();

		m_model_loader = std::make_unique<wr::AssimpModelLoader>();

		m_render_system->Init( window.get() );

		resources::CreateResources( m_render_system.get() );

		m_scenegraph = std::make_shared<wr::SceneGraph>( m_render_system.get() );

		m_viewport_camera = m_scenegraph->CreateChild<wr::CameraNode>( nullptr, 90.f, ( float )window->GetWidth() / ( float )window->GetHeight() );
		m_viewport_camera->SetPosition( { 0, 0, -1 } );

		SCENE::CreateScene( m_scenegraph.get(), window.get() );

		m_render_system->InitSceneGraph( *m_scenegraph.get() );

		m_framegraph = std::make_unique<wr::FrameGraph>( 4 );
		wr::AddDeferredMainTask( *m_framegraph, std::nullopt, std::nullopt );
		wr::AddDeferredCompositionTask( *m_framegraph, std::nullopt, std::nullopt );
		wr::AddRenderTargetReadBackTask<wr::DeferredCompositionTaskData>(*m_framegraph, std::nullopt, std::nullopt);
		wr::AddRenderTargetCopyTask<wr::DeferredCompositionTaskData>( *m_framegraph );
		auto render_editor = [&]()
		{
			engine::RenderEngine( m_render_system.get(), m_scenegraph.get() );
		};

		auto imgui_task = wr::GetImGuiTask( render_editor );
		
		m_framegraph->AddTask<wr::ImGuiTaskData>( imgui_task );
		m_framegraph->Setup( *m_render_system );
	}

	MHWRender::DrawAPI ViewportRenderer::supportedDrawAPIs() const
	{
		return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
	}

	MHWRender::MRenderOperation* ViewportRenderer::renderOperation()
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

	void ViewportRenderer::SynchronizeWispWithMayaViewportCamera()
	{
		M3dView view;
		MStatus status = M3dView::getM3dViewFromModelPanel( wisp::settings::VIEWPORT_PANEL_NAME, view );

		if( status != MStatus::kSuccess )
		{
			// Failure means no camera data for this frame, early-out!
			return;
		}

		// Model view matrix
		MMatrix mv_matrix;
		view.modelViewMatrix( mv_matrix );

		MDagPath camera_dag_path;
		view.getCamera( camera_dag_path );

		// Additional functionality
		MFnCamera camera_functions( camera_dag_path );

		MVector center = camera_functions.centerOfInterestPoint( MSpace::kWorld );
		MVector eye = camera_functions.eyePoint( MSpace::kWorld );

		m_viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
		m_viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();

		m_viewport_camera->SetFov( camera_functions.horizontalFieldOfView() );

		// Convert the MMatrix into an XMMATRIX and update the view matrix of the Wisp camera
		DirectX::XMFLOAT4X4 converted_mv_matrix;
		mv_matrix.get( converted_mv_matrix.m );
		m_viewport_camera->m_view = DirectX::XMLoadFloat4x4( &converted_mv_matrix );

		m_viewport_camera->SetPosition( { ( float )-eye.x, ( float )eye.y, ( float )-eye.z } );
	}

	MStatus ViewportRenderer::setup(const MString& destination)
	{
		SynchronizeWispWithMayaViewportCamera();
		SCENE::UpdateScene();

		auto texture = m_render_system->Render( m_scenegraph, *m_framegraph );

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
		if (!UpdateTextures(maya_renderer, maya_texture_manager, texture))
		{
			assert( false );
			return MStatus::kFailure;
		}

		// Force the panel display style to smooth shaded if it is not already
		// this ensures that viewport selection behavior works as if shaded
		EnsurePanelDisplayShading(destination);

		return MStatus::kSuccess;
	}

	bool ViewportRenderer::AreAllRenderOperationsSetCorrectly() const
	{
		return (!m_render_operations[0] ||
				!m_render_operations[1] ||
				!m_render_operations[2] ||
				!m_render_operations[3]);
	}

	MStatus ViewportRenderer::cleanup()
	{
		m_current_render_operation = -1;
		return MStatus::kSuccess;
	}

	MString ViewportRenderer::uiName() const
	{
		return m_ui_name;
	}

	bool ViewportRenderer::startOperationIterator()
	{
		m_current_render_operation = 0;
		return true;
	}

	bool ViewportRenderer::nextRenderOperation()
	{
		++m_current_render_operation;

		if (m_current_render_operation < 4)
		{
			return true;
		}

		return false;
	}

	static void loadImageFromDisk( MString& image_location, MHWRender::MTextureDescription& color_texture_desc, MHWRender::MTextureAssignment& color_texture, MHWRender::MTextureManager* texture_manager )
	{
		unsigned char* texture_data = nullptr;

		MImage image;

		unsigned int target_width, target_height;

		image.readFromFile( image_location );
		image.getSize( target_width, target_height );

		texture_data = image.pixels();

		color_texture_desc.fWidth = target_width;
		color_texture_desc.fHeight = target_height;
		color_texture_desc.fDepth = 1;
		color_texture_desc.fBytesPerRow = 4 * target_width;
		color_texture_desc.fBytesPerSlice = color_texture_desc.fBytesPerRow * target_height;

		// Acquire a new texture
		color_texture.texture = texture_manager->acquireTexture( "", color_texture_desc, texture_data );

		if( color_texture.texture )
		{
			color_texture.texture->textureDescription( color_texture_desc );
		}
	}

	bool ViewportRenderer::UpdateTextures(MHWRender::MRenderer* maya_renderer, MHWRender::MTextureManager* texture_manager, const wr::CPUTexture& cpu_texture)
	{
		if (!maya_renderer || !texture_manager)
			return false;

		std::stringstream strs;
		strs << "Width: " << cpu_texture.m_buffer_width << " Height: " << cpu_texture.m_buffer_height << " BBP: " << cpu_texture.m_bytes_per_pixel << std::endl;
		MGlobal::displayInfo(std::string(strs.str()).c_str());

		// Early exit, no texture data from Wisp available just yet
		if (cpu_texture.m_buffer_width == 0 ||
			cpu_texture.m_buffer_height == 0 ||
			cpu_texture.m_bytes_per_pixel == 0)
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
			(m_color_texture_desc.fWidth != target_width || m_color_texture_desc.fHeight != target_height))
		{
			if (m_color_texture.texture)
			{
				texture_manager->releaseTexture(m_color_texture.texture);
				m_color_texture.texture = NULL;
			}
			
			aquire_new_texture = true;
			force_reload = false;
		}

		if (!m_color_texture.texture)
		{
			// Grab the Wisp texture data
			auto* texture_data = new unsigned char[cpu_texture.m_buffer_width * cpu_texture.m_buffer_height * cpu_texture.m_bytes_per_pixel];
			memcpy(texture_data, cpu_texture.m_data, cpu_texture.m_buffer_width * cpu_texture.m_buffer_height * cpu_texture.m_bytes_per_pixel);

			m_color_texture_desc.fWidth = cpu_texture.m_buffer_width;
			m_color_texture_desc.fHeight = cpu_texture.m_buffer_height;
			m_color_texture_desc.fDepth = 1;
			m_color_texture_desc.fBytesPerRow = cpu_texture.m_bytes_per_pixel * cpu_texture.m_buffer_width;
			m_color_texture_desc.fBytesPerSlice = m_color_texture_desc.fBytesPerRow * cpu_texture.m_buffer_height;

			// Acquire a new texture.
			m_color_texture.texture = texture_manager->acquireTexture("", m_color_texture_desc, texture_data);

			if (m_color_texture.texture)
				m_color_texture.texture->textureDescription(m_color_texture_desc);

			// Don't need the data any more after upload to texture
			delete[] texture_data;
		}
		else
		{
			unsigned char* texture_data = nullptr;

			// Grab the Wisp texture data
			texture_data = new unsigned char[cpu_texture.m_buffer_width * cpu_texture.m_buffer_height * cpu_texture.m_bytes_per_pixel];

			m_color_texture.texture->update(texture_data, false);
			delete[] texture_data;
		}

		// Update the textures used for the blit operation.
		//
		if (aquire_new_texture)
		{
			auto* custom_blit = (WispScreenBlitter*)m_render_operations[0].get();
			custom_blit->SetColorTexture(m_color_texture);
		}

		return m_color_texture.texture ? true : false;
	}
}
