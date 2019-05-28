#include "scene_graph_parser.hpp"

#include "plugin/callback_manager.hpp"
#include "plugin/renderer/renderer.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/parsers/model_parser.hpp"
#include "plugin/parsers/material_parser.hpp"
#include "plugin/parsers/camera_parser.hpp"
#include "plugin/parsers/light_parser.hpp"

#include "model_pool.hpp"
#include "vertex.hpp"
#include "renderer.hpp"
#include "scene_graph/scene_graph.hpp"
#include "scene_graph/mesh_node.hpp"
#include "d3d12/d3d12_renderer.hpp"
#include "d3d12/d3d12_model_pool.hpp" 
#include "util/log.hpp"

#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnCamera.h>
#include <maya/MFnMesh.h>
#include <maya/MFnLight.h>
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
	if (node.apiType() != MFn::Type::kMesh)
	{
		LOGC("Trying to add mesh callback, but node type is not of \"kMesh\".");
	}

  	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetModelParser().SubscribeObject( node );

	LOG("New mesh added via MeshAddedCallback: \"{}\".", MFnMesh(node).fullPathName().asChar());
}

void MeshRemovedCallback( MObject& node, void* client_data )
{
	if (node.apiType() != MFn::Type::kMesh)
	{
		LOGC("Trying to remove mesh callback, but node type is not of \"kMesh\", it is of type \"{}\".", node.apiTypeStr());
	}

	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetModelParser().UnSubscribeObject( node );
}

void LightAddedCallback( MObject &node, void *client_data )
{
	if (!node.hasFn(MFn::Type::kLight))
	{
		LOGC("Trying to add light callback to {}, but node type is not of \"kLight\".", MFnLight(node).fullPathName().asChar());
	}

	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetLightParser().SubscribeObject( node );
}

void LightRemovedCallback( MObject& node, void* client_data )
{
	if (!node.hasFn(MFn::Type::kLight))
	{
		LOGC("Trying to remove light callback from \"{}\", but node type is not of \"kLight\".", MFnLight(node).fullPathName().asChar());
	}

	wmr::ScenegraphParser* scenegraph_parser = reinterpret_cast< wmr::ScenegraphParser* >( client_data );

	// Create an attribute changed callback to use in order to wait for the mesh to be ready
	scenegraph_parser->GetLightParser().UnSubscribeObject( node );
}

void ConnectionAddedCallback(MPlug& src_plug, MPlug& dest_plug, bool made, void* client_data)
{
 	auto* scenegraph_parser = reinterpret_cast<wmr::ScenegraphParser*>(client_data);
	auto* material_parser = &scenegraph_parser->GetMaterialParser();
	auto* model_parser = &scenegraph_parser->GetModelParser();

	// Get plug types
	auto src_type = src_plug.node().apiType();
	auto dest_type = dest_plug.node().apiType();

	// ============== CATCHING MATERIAL CONNECTIONS ==============
	switch (dest_type)
	{
		// Check if anything is bound to a shading engine
		// In that case, a material is either added or moved
		case MFn::kShadingEngine: {
			// Get destination object from destination plug
			MObject dest_object = dest_plug.node();

			// Bind the mesh to the shading engine if the source plug is a mesh
			switch (src_type)
			{
				case MFn::kMesh:
				{
					MObject src_object(src_plug.node());
					// Check if connection is made
					if (made)
					{
						material_parser->ConnectMeshToShadingEngine(src_object, dest_object);
					}
					else
					{
						material_parser->DisconnectMeshFromShadingEngine(src_object, dest_object);
					}
					break;
				}
				
				default:
				{
					// Get shader type of source plug
					auto shaderType = material_parser->GetShaderType(src_plug.node());
					// The type is UNSUPPORTED if we don't support it or if it's not a surface shader
					if (shaderType != wmr::detail::SurfaceShaderType::UNSUPPORTED)
					{
						// Check if connection is made
						if (made)
						{
							material_parser->ConnectShaderToShadingEngine(src_plug, dest_object);
						}
						else
						{
							material_parser->DisconnectShaderFromShadingEngine(src_plug, dest_object);
						}
					}
					break;
				}
			}
			break;
		}


		// When the destination plug is a shader list, a material is either made or removed
		case MFn::kShaderList:
		{
			// Get shader type of source plug
			auto shaderType = material_parser->GetShaderType(src_plug.node());
			// The type is UNSUPPORTED if we don't support it or if it's not a surface shader
			if (shaderType != wmr::detail::SurfaceShaderType::UNSUPPORTED)
			{
				if (made)
				{
					material_parser->OnCreateSurfaceShader(src_plug);
				}
				else
				{
					material_parser->OnRemoveSurfaceShader(src_plug);
				}
			}
			break;
		}


		// ============== CATCHING MESH CONNECTIONS ==============
		// Catch unite and boolean operations
		case MFn::kPolyCBoolOp:
		case MFn::kPolyUnite:
		{
			if (src_type == MFn::kMesh) {
				// Toggle the visibility of a mesh by specifiying if the connection was made or broken.
				model_parser->ToggleMeshVisibility(src_plug, made);
			}
			break;
		}

	}
}

wmr::ScenegraphParser::ScenegraphParser( ) :
	m_render_system( dynamic_cast< const ViewportRendererOverride* >(
		MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME )
		)->GetRenderer() )
{
	LOG("Attempting to get a reference to the renderer.");

	m_camera_parser = std::make_unique<CameraParser>();
	m_model_parser = std::make_unique<ModelParser>();
	m_light_parser = std::make_unique<LightParser>();
	m_material_parser = std::make_unique<MaterialParser>();
}

wmr::ScenegraphParser::~ScenegraphParser()
{
}

void wmr::ScenegraphParser::Initialize()
{
	m_camera_parser->Initialize();

	m_model_parser->SetMeshAddCallback([this] (MFnMesh & mesh)
	{
		this->m_material_parser->OnMeshAdded(mesh);
	});

	MStatus status;
	// Mesh added
	MCallbackId addedId = MDGMessage::addNodeAddedCallback(
		MeshAddedCallback,
		"mesh",
		this,
		&status
	);
	AddCallbackValidation(status, addedId);

	// Mesh removed 
	addedId = MDGMessage::addNodeRemovedCallback(
		MeshRemovedCallback,
		"mesh",
		this,
		&status
	);
	AddCallbackValidation(status, addedId);

	// Connection added (material)
	addedId = MDGMessage::addConnectionCallback(
		ConnectionAddedCallback,
		this,
		&status
	);
	AddCallbackValidation(status, addedId);

	addedId = MDGMessage::addNodeAddedCallback(
		LightAddedCallback,
		"light",
		this,
		&status
	);
	AddCallbackValidation(status, addedId);

	addedId = MDGMessage::addNodeRemovedCallback(
		LightRemovedCallback,
		"light",
		this,
		&status
	);
	AddCallbackValidation(status, addedId);
	
	// TODO: add other types of addedCallbacks

	//load meshes
	MStatus load_status = MS::kSuccess;
	MItDag mesh_itt( MItDag::kDepthFirst, MFn::kMesh, &load_status );
	if( load_status != MS::kSuccess )
	{
		LOGE("Could not get a mesh iterator when loading meshes.");
		MGlobal::displayError( "false to get iterator: " + load_status );
	}

	while( !mesh_itt.isDone() )
	{
		MFnMesh mesh( mesh_itt.currentItem() );
		if( !mesh.isIntermediateObject() )
		{
			m_model_parser->MeshAdded(mesh);
			m_material_parser->OnMeshAdded(mesh);
		}
		mesh_itt.next();
	}

	//load lights
	load_status = MS::kSuccess;

	MItDag light_itt( MItDag::kDepthFirst, MFn::kLight, &load_status );

	if( load_status != MS::kSuccess )
	{
		LOGE("Could not get a light iterator when loading lights.");
		MGlobal::displayError( "false to get iterator: " + load_status );
	}

	while( !light_itt.isDone() )
	{
		MFnLight light( light_itt.currentItem() );
		m_light_parser->LightAdded( light );
		light_itt.next();
	}
}

void wmr::ScenegraphParser::Update()
{
	m_model_parser->Update();
}

void wmr::ScenegraphParser::AddCallbackValidation(MStatus status, MCallbackId id)
{
	if (status == MS::kSuccess)
	{
		CallbackManager::GetInstance().RegisterCallback(id);
	}
	else
	{
		LOGC("Callback coud not be added, it is not valid.");
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

wmr::LightParser & wmr::ScenegraphParser::GetLightParser() const noexcept
{
	return *m_light_parser;
}
