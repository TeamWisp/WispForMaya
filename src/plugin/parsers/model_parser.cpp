#include "model_parser.hpp"

#include "plugin/callback_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/renderer/model_manager.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/texture_manager.hpp"
#include "plugin/renderer/material_manager.hpp"

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
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MStatus.h>
#include <maya/M3dView.h>
#include <maya/MFnDagNode.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MUuid.h>
#include <maya/MDGMessage.h>

#include <string>

// region for internally used functions, these functions cannot be use outside this cpp file
#pragma region INTERNAL_FUNCTIONS
static MIntArray GetLocalIndex( MIntArray & getVertices, MIntArray & getTriangle )
{
	// MItMeshPolygon::getTriangle() returns object-relative vertex indices
	// BUT MItMeshPolygon::normalIndex() and ::getNormal() need face-relative vertex indices! 
	// This helper function converts vertex indices from object-relative to face-relative.

	assert( getTriangle.length() == 3 );    // We should always handle triangles and nothing else

	MIntArray   localIndex( 3 );
	unsigned    gv, gt;

	for( gt = 0; gt < getTriangle.length(); gt++ )
	{
		localIndex[gt] = -1;
		for( gv = 0; gv < getVertices.length(); gv++ )
		{
			if( getTriangle[gt] == getVertices[gv] )
			{
				localIndex[gt] = gv;
				break;
			}
		}
	}
	return localIndex;
}

static MMatrix getParentWorldMatrix(MFnTransform& trans)
{
	auto count = trans.parentCount();
	if( count > 0 )
	{
		MFnDagNode dagnode = trans.parent( 0 );
		MDagPath path;
		dagnode.getPath( path );
		auto parent = MFnTransform( path );
		return parent.transformation().asMatrix() * getParentWorldMatrix( parent );
	}
	else
	{
		return MMatrix::identity;
	}
}

static void updateTransform( MFnTransform& transform, std::shared_ptr<wr::MeshNode> mesh_node )
{
	MStatus status = MS::kSuccess;
	MDagPath path;
	MDagPath::getAPathTo( transform.object(), path );
	MFnTransform good_trans( path, &status);

	if (status != MS::kSuccess)
	{
		LOGC("Could not get a good transform when trying to update the transform.");
	}

	auto parent_matrix = getParentWorldMatrix( good_trans );
	auto child_matrix = good_trans.transformationMatrix();
	auto child_world_matrix = child_matrix * parent_matrix;
	
	MTransformationMatrix child_trans_matrix( child_world_matrix);

	MVector pos = child_trans_matrix.translation(MSpace::kWorld);

	double4 quatd = { 0,0,0,0 };
	status = child_trans_matrix.getRotationQuaternion( quatd[0], quatd[1], quatd[2], quatd[3], MSpace::kObject );
	
	double3 scale;
	status = child_trans_matrix.getScale( scale, MSpace::kWorld );

	if (status != MS::kSuccess)
	{
		LOGC("Could not get transform data.");
	}

	mesh_node->SetPosition( { static_cast< float >( pos.x ), static_cast< float >( pos.y ), static_cast< float >( pos.z ) } );
	mesh_node->SetQuaternionRotation( quatd[0], quatd[1], quatd[2], quatd[3] );
	mesh_node->SetScale( { static_cast< float >( scale[0] ), static_cast< float >( scale[1] ),static_cast< float >( scale[2] ) } );
}

auto getTransformFindAlgorithm( MFnTransform& transform )
{
	return [ &transform ]( std::pair<MObject, std::shared_ptr<wr::MeshNode>> pair ) -> bool
	{
		MStatus status;
		MFnMesh fn_mesh( pair.first );
		MFnDagNode dagnode = fn_mesh.parent( 0, &status );
		MObject object = dagnode.object();
		MFnTransform transform_rhs( dagnode.object(), &status );

		assert( status == MS::kSuccess );

		if( transform.object() == transform_rhs.object() )
		{
			return true;
		}
		return false;
	};
}

auto getMeshObjectAlgorithm(MObject& maya_object)
{
	return [ &maya_object ]( std::pair<MObject, std::shared_ptr<wr::MeshNode>> pair ) -> bool
	{
		if( maya_object == pair.first )
		{
			return true;
		}
		return false;
	};
}

void parseData( MFnMesh & fnmesh, wr::MeshData<wr::Vertex>& mesh_data )
{
	MPointArray meshPoints;
	fnmesh.getPoints( meshPoints, MSpace::kObject );

	//  Cache normals for each vertex
	MFloatVectorArray  meshNormals;

	// Normals are per-vertex per-face..
	// use MItMeshPolygon::normalIndex() for index
	fnmesh.getNormals( meshNormals );

	// Get UVSets for this mesh
	MStringArray  UVSets;
	MStatus status = fnmesh.getUVSetNames( UVSets );

	// Get all UV coordinates for the first UV set (default "map1").
	MFloatArray   u, v;
	fnmesh.getUVs( u, v, &UVSets[0] );

	MFloatVectorArray  meshTangents;
	fnmesh.getTangents( meshTangents );

	MFloatVectorArray  meshBinormals;
	fnmesh.getBinormals( meshBinormals );

	mesh_data.m_indices = std::make_optional( std::vector<uint32_t>() );

	MItMeshPolygon itt( fnmesh.object(), &status );

	// hacky way. indices just count up. <-------------------------
	uint32_t indices_hack = 0;

	while( !itt.isDone() )
	{
		// Get object-relative indices for the vertices in this face.
		MIntArray polygonVertices;
		itt.getVertices( polygonVertices );

		// Get triangulation of this polygon
		int numTriangles;
		itt.numTriangles( numTriangles );

		// needed for function but never used.
		MPointArray nonTweaked;

		// object-relative vertex indices for each triangle
		MIntArray triangleVertices;

		// face-relative vertex indices for each triangle
		MIntArray localIndex;

		for( size_t i = 0; i < numTriangles; i++ )
		{
			status = itt.getTriangles( nonTweaked,
				triangleVertices,
				MSpace::kObject );

			MIntArray firstTriangleVertices;

			firstTriangleVertices.setLength( 3 );

			firstTriangleVertices[0] = triangleVertices[0 + 3 * i];
			firstTriangleVertices[1] = triangleVertices[1 + 3 * i];
			firstTriangleVertices[2] = triangleVertices[2 + 3 * i];

			if( status == MS::kSuccess )
			{

				// TODO(jens): hash resolved vertices to check if there are duplicates. start using indices (properly)

				wr::Vertex v1;
				wr::Vertex v2;
				wr::Vertex v3;

				v1.m_pos[0] = ( float )meshPoints[firstTriangleVertices[0]].x;
				v1.m_pos[1] = ( float )meshPoints[firstTriangleVertices[0]].y;
				v1.m_pos[2] = ( float )meshPoints[firstTriangleVertices[0]].z;

				v2.m_pos[0] = ( float )meshPoints[firstTriangleVertices[1]].x;
				v2.m_pos[1] = ( float )meshPoints[firstTriangleVertices[1]].y;
				v2.m_pos[2] = ( float )meshPoints[firstTriangleVertices[1]].z;

				v3.m_pos[0] = ( float )meshPoints[firstTriangleVertices[2]].x;
				v3.m_pos[1] = ( float )meshPoints[firstTriangleVertices[2]].y;
				v3.m_pos[2] = ( float )meshPoints[firstTriangleVertices[2]].z;

				localIndex = GetLocalIndex( polygonVertices, firstTriangleVertices );

				v1.m_normal[0] = meshNormals[itt.normalIndex( localIndex[0] )].x;
				v1.m_normal[1] = meshNormals[itt.normalIndex( localIndex[0] )].y;
				v1.m_normal[2] = meshNormals[itt.normalIndex( localIndex[0] )].z;

				v2.m_normal[0] = meshNormals[itt.normalIndex( localIndex[1] )].x;
				v2.m_normal[1] = meshNormals[itt.normalIndex( localIndex[1] )].y;
				v2.m_normal[2] = meshNormals[itt.normalIndex( localIndex[1] )].z;

				v3.m_normal[0] = meshNormals[itt.normalIndex( localIndex[2] )].x;
				v3.m_normal[1] = meshNormals[itt.normalIndex( localIndex[2] )].y;
				v3.m_normal[2] = meshNormals[itt.normalIndex( localIndex[2] )].z;

				v1.m_tangent[0] = meshTangents[itt.tangentIndex( localIndex[0] )].x;
				v1.m_tangent[1] = meshTangents[itt.tangentIndex( localIndex[0] )].y;
				v1.m_tangent[2] = meshTangents[itt.tangentIndex( localIndex[0] )].z;

				v2.m_tangent[0] = meshTangents[itt.tangentIndex( localIndex[1] )].x;
				v2.m_tangent[1] = meshTangents[itt.tangentIndex( localIndex[1] )].y;
				v2.m_tangent[2] = meshTangents[itt.tangentIndex( localIndex[1] )].z;

				v3.m_tangent[0] = meshTangents[itt.tangentIndex( localIndex[2] )].x;
				v3.m_tangent[1] = meshTangents[itt.tangentIndex( localIndex[2] )].y;
				v3.m_tangent[2] = meshTangents[itt.tangentIndex( localIndex[2] )].z;

				v1.m_bitangent[0] = meshBinormals[itt.normalIndex( localIndex[0] )].x;
				v1.m_bitangent[1] = meshBinormals[itt.normalIndex( localIndex[0] )].y;
				v1.m_bitangent[2] = meshBinormals[itt.normalIndex( localIndex[0] )].z;

				v2.m_bitangent[0] = meshBinormals[itt.normalIndex( localIndex[1] )].x;
				v2.m_bitangent[1] = meshBinormals[itt.normalIndex( localIndex[1] )].y;
				v2.m_bitangent[2] = meshBinormals[itt.normalIndex( localIndex[1] )].z;

				v3.m_bitangent[0] = meshBinormals[itt.normalIndex( localIndex[2] )].x;
				v3.m_bitangent[1] = meshBinormals[itt.normalIndex( localIndex[2] )].y;
				v3.m_bitangent[2] = meshBinormals[itt.normalIndex( localIndex[2] )].z;

				int firstUVID[3];

				// Get UV values for each vertex within this polygon
				for( int vtxInPolygon = 0; vtxInPolygon < 3; vtxInPolygon++ )
				{
					itt.getUVIndex( localIndex[vtxInPolygon],
						firstUVID[vtxInPolygon],
						&UVSets[0] );
				}

				v1.m_uv[0] = u[firstUVID[0]];
				v1.m_uv[1] = v[firstUVID[0]];

				v2.m_uv[0] = u[firstUVID[1]];
				v2.m_uv[1] = v[firstUVID[1]];

				v3.m_uv[0] = u[firstUVID[2]];
				v3.m_uv[1] = v[firstUVID[2]];

				mesh_data.m_vertices.push_back( v3 );
				mesh_data.m_vertices.push_back( v2 );
				mesh_data.m_vertices.push_back( v1 );


				// hacky way. indices just count up. <-------------------------
				mesh_data.m_indices.value().push_back( indices_hack );
				indices_hack++;
				mesh_data.m_indices.value().push_back( indices_hack );
				indices_hack++;
				mesh_data.m_indices.value().push_back( indices_hack );
				indices_hack++;
			}
			else
			{
				LOGC("Could not get the triangles.");
				MGlobal::displayInfo( status.errorString() );
			}

		}
		itt.next();
	}
}
#pragma endregion

#pragma region callbacks
namespace wmr
{
	void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data )
	{
		// Check if attribute was set
		if( !( msg & MNodeMessage::kAttributeSet ) )
		{
			return;
		}

		MStatus status = MS::kSuccess;
		MFnTransform transform( plug.node(), &status );
		if( status != MS::kSuccess )
		{
			MGlobal::displayInfo( status.errorString() );
			return;
		}
		wmr::ModelParser* model_parser = reinterpret_cast< wmr::ModelParser* >( client_data );

		// specialized find_if algorithm
		auto it = std::find_if( model_parser->m_object_transform_vector.begin(), model_parser->m_object_transform_vector.end(), getTransformFindAlgorithm( transform ) );
		if( it == model_parser->m_object_transform_vector.end() )
		{
			return;
		}
		MFnMesh fn_mesh( it->first );
		MFnDagNode dagnode = fn_mesh.parent( 0, &status );
		MObject object = dagnode.object();
		MFnTransform transform_rhs( dagnode.object(), &status );
		if( transform_rhs.object() != transform.object() )
		{
			return; // find_if returns last element even if it is not a positive result
		}

		updateTransform( transform, it->second );

		auto child_count = transform.childCount();
		if( child_count < 2 )
		{
			return;
		}
		for( unsigned int i = 0; i < child_count; ++i )
		{
			status = MS::kSuccess;
			MFnTransform child_transform(transform.child( i ), &status);
			if( status == MS::kSuccess )
			{
				MPlug child_plug( child_transform.object(), plug.attribute() );
				AttributeMeshTransformCallback( msg, child_plug , other_plug, client_data );
			}
		}
	}

	void AttributeMeshAddedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data )
	{
		// Make an MObject from the plug node
		MObject object( plug.node() );

		// Maya adds "__PrenotatoPerDuplicare_" in front of a temporary mesh duplicate, those objects can be ignored
		// We only care about the final object
		auto dep_node_fn = MFnDependencyNode(object);

		std::string node_name = dep_node_fn.name().asChar();
		auto result = node_name.find(settings::INTERMEDIATE_MESH_IGNORE_PREFIX);

		// Ignore temporary objects
		if (result != std::string::npos &&
			result == 0)
		{
			return;
		}

		MStatus status = MS::kSuccess;
		// Create mesh
		MFnMesh mesh( object, &status );
		if( status != MS::kSuccess )
		{
			return;
		}

		wmr::ModelParser* model_parser = reinterpret_cast< wmr::ModelParser* >( client_data );
		// Add the mesh
		model_parser->MeshAdded(mesh);

		if (model_parser->mesh_add_callback != nullptr)
		{
			model_parser->mesh_add_callback(mesh);
		}

		// Unregister the callback
		auto findCallback = [ &object ]( std::pair<MObject, MCallbackId> pair ) -> bool
		{
			if( object == pair.first )
			{
				return true;
			}
			return false;
		};

		std::vector<std::pair<MObject, MCallbackId>>::iterator it =
			std::find_if( model_parser->m_mesh_added_callback_vector.begin(), model_parser->m_mesh_added_callback_vector.end(), findCallback );

		if (model_parser->m_mesh_added_callback_vector.size() <= 0)
		{
			LOGC("Mesh added callback vector size is zero.");
		}

		auto it_end = --model_parser->m_mesh_added_callback_vector.end();

		if( it == it_end )
		{
			if (findCallback(*it))
			{
				LOGC("Callback was never added to the mesh added callback vector.");
			}

			MMessage::removeCallback( it_end->second );
			model_parser->m_mesh_added_callback_vector.pop_back();

		}
		else
		{
			std::iter_swap( it, it_end );
			MMessage::removeCallback( it_end->second );
			model_parser->m_mesh_added_callback_vector.pop_back();
		}

	}

	void attributeMeshChangedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data )
	{
		if( !( msg & MNodeMessage::kAttributeSet ) )
		{
			return;
		}
		
		MStatus status = MS::kSuccess;
		
		// Was the attribute a point?
		MPlug vertexPlug = MFnDependencyNode( plug.node() ).findPlug( "pnts", &status );
		if( status != MS::kSuccess )
		{
			return;
		}
		
		int index = plug.logicalIndex();
		
		// In some rare cases, the logic index could be -1 and we want to check this as well
		if( index != -1 )
		{
			return;
		}

		wmr::ModelParser* model_parser = reinterpret_cast< wmr::ModelParser* >( client_data );

		MObject object( plug.node() );
		auto itt = std::find( model_parser->m_changed_mesh_vector.begin(), model_parser->m_changed_mesh_vector.end() , object);
		if( itt == model_parser->m_changed_mesh_vector.end() )
		{
			model_parser->m_changed_mesh_vector.push_back(object);
		}
	}
}
#pragma endregion

wmr::ModelParser::ModelParser() :
	m_renderer( dynamic_cast< const ViewportRendererOverride* >(
		MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME )
		)->GetRenderer() ),
	m_mesh_added_callback_vector(),
	m_object_transform_vector(),
	m_changed_mesh_vector()
{
}

wmr::ModelParser::~ModelParser()
{
}

void wmr::ModelParser::SubscribeObject( MObject & maya_object )
{
	MStatus status = MS::kSuccess;

	// For this callback, we want to use a temporary functions to gather data from a mesh when it's added to the scene
	auto meshCreatedID = MNodeMessage::addAttributeChangedCallback(
		maya_object,
		AttributeMeshAddedCallback,
		this,
		&status
	);

	if (status != MS::kSuccess)
	{
		LOGC("Could not subscribe object to attribute changed callback.");
	}

	m_mesh_added_callback_vector.push_back( std::make_pair( maya_object, meshCreatedID ) );

}

void wmr::ModelParser::UnSubscribeObject( MObject & maya_object )
{
	MStatus status = MS::kSuccess;

	MFnMesh fnmesh( maya_object );

	
	auto it = std::find_if( m_object_transform_vector.begin(), m_object_transform_vector.end(), getMeshObjectAlgorithm(maya_object) );
	if( it == m_object_transform_vector.end() )
	{
		LOGC("Iterator past end of object transform vector.");
		return; // find_if returns last element even if it is not a positive result
	}
	m_renderer.GetScenegraph().DestroyNode( it->second );

	if (m_object_transform_vector.empty())
	{
		LOGC("Object transform vector is empty.");
	}

	auto it_end = --m_object_transform_vector.end();

	if( it == it_end )
	{
		m_object_transform_vector.pop_back();
	}
	else
	{
		std::iter_swap( it, it_end );
		m_object_transform_vector.pop_back();
	}


}

void wmr::ModelParser::MeshAdded( MFnMesh & fnmesh )
{
	wr::MeshData<wr::Vertex> mesh_data;
	
	parseData( fnmesh, mesh_data );

	bool model_reloaded = false;
	wr::Model* model = m_renderer.GetModelManager().AddModel( fnmesh.name(), { mesh_data }, model_reloaded );
	m_renderer.GetD3D12Renderer().WaitForAllPreviousWork();
	auto model_node = m_renderer.GetScenegraph().CreateChild<wr::MeshNode>( nullptr, model );
	MStatus status;


	//CallbackManager::GetInstance().RegisterCallback()

	MFnDagNode dagnode = fnmesh.parent( 0, &status );
	if( status != MS::kSuccess )
	{
		LOGE("Could not get the mesh dagnode.");
		MGlobal::displayError( "Error: " + status.errorString() );
	}

	MObject object = dagnode.object();


	MFnTransform transform( dagnode.object(), &status );
	if( status != MS::kSuccess )
	{
		LOGE("Could not get transform node function set.");
		MGlobal::displayError( "Error: " + status.errorString() );
	}

	updateTransform( transform, model_node );

	m_object_transform_vector.push_back( std::make_pair( fnmesh.object(), model_node ) );


	MCallbackId attributeId = MNodeMessage::addAttributeChangedCallback(
		object,
		AttributeMeshTransformCallback,
		this,
		&status
	);
	CallbackManager::GetInstance().RegisterCallback( attributeId );

	MObject mesh_obj = fnmesh.object();

	attributeId = MNodeMessage::addAttributeChangedCallback(
		mesh_obj,
		attributeMeshChangedCallback,
		this,
		&status
	);
	CallbackManager::GetInstance().RegisterCallback( attributeId );

}

void wmr::ModelParser::Update()
{
	for( auto& object : m_changed_mesh_vector )
	{
		MStatus status = MS::kSuccess;
		MFnMesh fn_mesh( object, &status );
		if( status != MS::kSuccess )
		{
			continue;
		}
		wr::MeshData<wr::Vertex> mesh_data;

		parseData( fn_mesh, mesh_data );

		auto itt = std::find_if( m_object_transform_vector.begin(), m_object_transform_vector.end(), getMeshObjectAlgorithm( object ) );
		if( itt == m_object_transform_vector.end() )
		{
			return; // find_if returns last element even if it is not a positive result
		}

		m_renderer.GetModelManager().UpdateModel( *itt->second->m_model , mesh_data );

	}
	m_changed_mesh_vector.clear();
}

void wmr::ModelParser::SetMeshAddCallback(std::function<void(MFnMesh&)> callback)
{
	if (mesh_add_callback != nullptr)
	{
		LOGC("Mesh added callback already set.");
	}
	mesh_add_callback = callback;
}

std::shared_ptr<wr::MeshNode> wmr::ModelParser::GetWRModel(MObject & maya_object)
{
	auto findCallback = [&maya_object] (std::pair<MObject, std::shared_ptr<wr::MeshNode>> pair) -> bool
	{
		if (maya_object == pair.first)
		{
			return true;
		}
		return false;
	};
	auto it = std::find_if(m_object_transform_vector.begin(), m_object_transform_vector.end(), findCallback);
	if (it != m_object_transform_vector.end())
	{
		return it->second;
	}
	return nullptr;//std::make_shared<wr::MeshNode>();
}
