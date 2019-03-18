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


void MeshAddedCallback( MObject &node, void *client_data )
{
	assert( node.apiType() == MFn::Type::kMesh );
	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetModelParser().SubscribeObject( node );
}

void MeshRemovedCallback( MObject& node, void* client_data )
{
	assert( node.apiType() == MFn::Type::kMesh );
	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetModelParser().UnSubscribeObject( node );
}

void MaterialAddedCallback(MObject& node, void* client_data)
{
	auto* material_parser = reinterpret_cast<wmr::MaterialParser*>(client_data);

	if (node.apiType() == MFn::Type::kMesh)
	{
		MStatus status = MStatus::kSuccess;

		// Get the DAG node
		MFnDagNode dag_node(node, &status);

		if (status == MStatus::kSuccess)
		{
			MFnMesh mesh(node);
			MGlobal::displayInfo("A material on the mesh \"" + dag_node.name() + "\" has been added!");
			material_parser->Parse(mesh);
		}
		else
		{
			MGlobal::displayInfo(status.errorString());
		}
	}
}

wmr::ScenegraphParser::ScenegraphParser( ) :
	m_render_system( dynamic_cast< const ViewportRendererOverride* >(
		MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME )
		)->GetRenderer() )
{
	m_camera_parser = std::make_unique<CameraParser>();
	m_model_parser = std::make_unique<ModelParser>();
	m_material_parser = std::make_unique<MaterialParser>();
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

	MCallbackId material_added_id = MDGMessage::addNodeAddedCallback(
		MaterialAddedCallback,
		"mesh",
		m_material_parser.get(),
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

	addedId = MDGMessage::addNodeRemovedCallback(
		MeshRemovedCallback,
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
		MGlobal::displayError( "false to get iterator: " + load_status );
	}

	while( !itt.isDone() )
	{
		MFnMesh mesh( itt.currentItem() );
		if( !mesh.isIntermediateObject() )
		{
			m_model_parser->MeshAdded( mesh );
			m_material_parser->Parse(mesh);
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
