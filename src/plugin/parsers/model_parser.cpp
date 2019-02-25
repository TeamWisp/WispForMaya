#include "model_parser.hpp"

#include "plugin/callback_manager.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "plugin/renderer/renderer.hpp"
#include "plugin/renderer/model_manager.hpp"
#include "miscellaneous/settings.hpp"
#include "plugin/renderer/texture_manager.hpp"

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

static void updateTransform( MFnTransform& transform, std::shared_ptr<wr::MeshNode> mesh_node )
{
	MStatus status = MS::kSuccess;
	MVector pos = transform.getTranslation( MSpace::kTransform, &status );

	MQuaternion qrot;
	status = transform.getRotation( qrot, MSpace::kTransform );
	qrot.normalizeIt();
	MEulerRotation rot = qrot.asEulerRotation();
	rot.reorderIt( MEulerRotation::kZXY );

	double3 scale;
	transform.getScale( scale );

	mesh_node->SetPosition( { -static_cast< float >( pos.x ), -static_cast< float >( pos.y ), -static_cast< float >( pos.z ) } );
	mesh_node->SetRotation( { static_cast< float >( rot.z ), static_cast< float >( rot.x ), static_cast< float >( rot.y ) } );
	mesh_node->SetScale( { static_cast< float >( scale[0] ), static_cast< float >( scale[1] ),static_cast< float >( scale[2] ) } );
}
#pragma endregion

#pragma region callbacks
namespace wmr
{
	void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData )
	{
		wmr::ModelParser* model_parser = reinterpret_cast< wmr::ModelParser* >( clientData );
		// Check if attribute was set
		if( msg & MNodeMessage::kAttributeSet )
		{
			MStatus status = MS::kSuccess;

			// Create transform from the MObject
			MFnTransform transform( plug.node(), &status );
			if( status == MS::kSuccess )
			{
				auto findTransform = [ &transform ]( std::pair<MObject, std::shared_ptr<wr::MeshNode>> pair ) -> bool
				{
					if( transform.object() == pair.first )
					{
						return true;
					}
					return false;
				};
				std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>>::iterator it =
					std::find_if( model_parser->m_object_transform_vector.begin(), model_parser->m_object_transform_vector.end(), findTransform );

				updateTransform( transform, it->second );
			}
			else
			{
				MGlobal::displayInfo( status.errorString() );
			}
		}
		else
		{
			return;
		}
	}
}
#pragma endregion

wmr::ModelParser::ModelParser() :
	m_render_system( dynamic_cast< const ViewportRendererOverride* >(
		MHWRender::MRenderer::theRenderer()->findRenderOverride( settings::VIEWPORT_OVERRIDE_NAME )
		)->GetRenderer() )
{
}

wmr::ModelParser::~ModelParser() 
{
}

void wmr::ModelParser::MeshAdded( MFnMesh & fnmesh )
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

	wr::MeshData<wr::Vertex> mesh_data;
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
				assert( false ); //could not get triangles
				MGlobal::displayInfo( status.errorString() );
			}

		}
		itt.next();
	}
	bool model_reloaded = false;
	wr::Model* model = m_render_system.GetModelManager().AddModel( fnmesh.name(), { mesh_data },model_reloaded );

	auto model_node = m_scenegraph.CreateChild<wr::MeshNode>( nullptr, model );
	//MStatus status;


	//CallbackManager::GetInstance().RegisterCallback()

	MFnDagNode dagnode = fnmesh.parent( 0, &status );
	if( status != MS::kSuccess )
	{
		MGlobal::displayError( "Error: " + status.errorString() );
	}

	MObject object = dagnode.object();


	MFnTransform transform( dagnode.object(), &status );
	if( status != MS::kSuccess )
	{
		MGlobal::displayError( "Error: " + status.errorString() );
	}

	updateTransform( transform, model_node );

	model->m_meshes[0].second = m_render_system.GetMaterialManager();

	m_object_transform_vector.push_back( std::make_pair( transform.object(), model_node ) );


	MCallbackId attributeId = MNodeMessage::addAttributeChangedCallback(
		object,
		AttributeMeshTransformCallback,
		this,
		&status
	);
	CallbackManager::GetInstance().RegisterCallback( attributeId );

}
