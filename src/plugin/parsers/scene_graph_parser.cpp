#include "scene_graph_parser.hpp"

#include "plugin/callback_manager.hpp"
#include "plugin/renderer/renderer.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/parsers/model_parser.hpp"
#include "plugin/parsers/material_parser.hpp"
#include "plugin/parsers/camera_parser.hpp"

#include "model_pool.hpp"
#include "vertex.hpp"
#include "renderer.hpp"
#include "scene_graph/scene_graph.hpp"
#include "scene_graph/mesh_node.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "d3d12/d3d12_model_pool.hpp" 


#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnCamera.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/M3dView.h>
#include <maya/MStatus.h>
#include <maya/MFnDagNode.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MUuid.h>
#include <maya/MDGMessage.h>

#include <sstream>


void MeshAddedCallback( MObject &node, void *clientData )
{
	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( clientData );

	// Check if the added node is a mesh
	if( node.apiType() == MFn::Type::kMesh )
	{
		// Create an attribute changed callback to use in order to wait for the mesh to be ready
		scenegraph_parser->GetModelParser().SubscribeObject( node );
	}
}

wmr::ScenegraphParser::ScenegraphParser( ) :
	m_render_system( dynamic_cast< const ViewportRendererOverride* >(
		MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME )
		)->GetRenderer() )
{
	m_camera_parser = std::make_unique<CameraParser>();
	m_model_parser = std::make_unique<ModelParser>();
}

wmr::ScenegraphParser::~ScenegraphParser()
{
}

void wmr::ScenegraphParser::Initialize()
{
	m_camera_parser->Initialize();

	MStatus status;

	MCallbackId addedId = MDGMessage::addNodeAddedCallback(
		MeshAddedCallback,
		"mesh",
		this,
		&status
	);
	
	if( status == MS::kSuccess )
	{
		CallbackManager::GetInstance().RegisterCallback( addedId );
	}
	else
	{
		assert( false );
	}
	
	// TODO: add other types of addedCallbacks

	MStatus load_status = MS::kSuccess;

	MItDag itt( MItDag::kDepthFirst, MFn::kMesh, &load_status );

	if( load_status != MS::kSuccess )
	{
		MGlobal::displayError( "false to get itterator: " + load_status );
	}

	while( !itt.isDone() )
	{
		MFnMesh mesh( itt.currentItem() );
		if( !mesh.isIntermediateObject() )
		{
			m_model_parser->MeshAdded( mesh );
			//add callback here <--------!!
		}
		itt.next();
	}
}

wmr::ModelParser & wmr::ScenegraphParser::GetModelParser() const noexcept
{
	return *m_model_parser;
}

wmr::MaterialParser & wmr::ScenegraphParser::GetMaterialParser() const noexcept
{
	return *m_material_parser;
}

wmr::CameraParser& wmr::ScenegraphParser::GetCameraParser() const noexcept
{
	return *m_camera_parser;
}
