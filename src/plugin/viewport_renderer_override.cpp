#include "viewport_renderer_override.hpp"

// Wisp plug-in
#include "miscellaneous/functions.hpp"
#include "miscellaneous/settings.hpp"
#include "parsers/scene_graph_parser.hpp"
#include "render_operations/gizmo_render_operation.hpp"
#include "render_operations/renderer_copy_operation.hpp"
#include "render_operations/renderer_draw_operation.hpp"
#include "render_operations/renderer_update_operation.hpp"
#include "render_operations/screen_render_operation.hpp"
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

		CreateRenderOperations();

		m_scenegraph_parser = std::make_unique<ScenegraphParser>();
		m_scenegraph_parser->Initialize();
	}

	ViewportRendererOverride::~ViewportRendererOverride()
	{
		// Not the Wisp renderer, but the internal Maya renderer
		const auto maya_renderer = MHWRender::MRenderer::theRenderer();

		if( maya_renderer )
		{
			// De-register the actual plug-in
			maya_renderer->deregisterOverride( this );
		}
	}

	void ViewportRendererOverride::CreateRenderOperations()
	{
		if (!m_render_operations[0])
		{
			m_render_operations[0] = std::make_unique<RendererUpdateOperation>	(settings::RENDER_OPERATION_NAMES[0]);
			m_render_operations[1] = std::make_unique<RendererDrawOperation>	(settings::RENDER_OPERATION_NAMES[1]);
			m_render_operations[2] = std::make_unique<RendererCopyOperation>	(settings::RENDER_OPERATION_NAMES[2]);
			m_render_operations[3] = std::make_unique<ScreenRenderOperation>	(settings::RENDER_OPERATION_NAMES[3]);
			m_render_operations[4] = std::make_unique<GizmoRenderOperation>		(settings::RENDER_OPERATION_NAMES[4]);
			m_render_operations[5] = std::make_unique<MHWRender::MHUDRender>	(settings::RENDER_OPERATION_NAMES[5]);
			m_render_operations[6] = std::make_unique<MHWRender::MPresentTarget>(settings::RENDER_OPERATION_NAMES[6]);
		}
	}

	MHWRender::DrawAPI ViewportRendererOverride::supportedDrawAPIs() const
	{
		return (MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
	}

	MHWRender::MRenderOperation* ViewportRendererOverride::renderOperation()
	{
		if (m_current_render_operation >= 0 && m_current_render_operation < settings::RENDER_OPERATION_COUNT)
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
		
		m_viewport_camera->SetFov( AI_RAD_TO_DEG( camera_functions.horizontalFieldOfView()) );
	}

	Renderer& ViewportRendererOverride::GetRenderer() const
	{
		return *m_renderer;
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

		// Force the panel display style to smooth shaded if it is not already
		// this ensures that viewport selection behavior works as if shaded
		EnsurePanelDisplayShading(destination);

		return MStatus::kSuccess;
	}

	bool ViewportRendererOverride::AreAllRenderOperationsSetCorrectly() const
	{
		bool all_good = true;

		for (const auto& operation : m_render_operations)
		{
			if (!operation)
			{
				all_good = false;
			}
		}

		return (all_good);
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

		if (m_current_render_operation < settings::RENDER_OPERATION_COUNT)
		{
			return true;
		}

		return false;
	}
}
