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
#include <chrono>

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

int32_t parseDataVertexCmp(uint32_t current_index, wr::Vertex *vertex, wr::Vertex* vertex_other) {
	if (memcmp(vertex, vertex_other, sizeof(wr::Vertex)) == 0) { // Vertex has found a duplicate!
		return current_index;
	}
	else { // Duplicate vertex has not been found (most often)
		return -1;
	}
}

using hash_type = unsigned long long;

const char checkVertexHash(hash_type * hashes, std::vector<hash_type> & v_hashes, uint32_t * found_index) {
	char c = 0;
	for (int32_t i = 0; i < v_hashes.size(); ++i) {
		if (v_hashes[i] == hashes[0]) {
			found_index[0] = i;
			c |= 1 << 0;
		}
		else if(v_hashes[i] == hashes[1]) {
			found_index[1] = i;
			c |= 1 << 1;
		}
		else if (v_hashes[i] == hashes[2]) {
			found_index[2] = i;
			c |= 1 << 2;
		}

		if (c & 0b111) {
			return c;
		}
	}
	return c;
}

hash_type compute_hash(int v, int n, int u, int t) {
	const hash_type m = 1e9 + 9;
	hash_type hash_value = 0;

	hash_value = (hash_value + hash_type(v + 1) * 1LL) % m;
	hash_value = (hash_value + hash_type(n + 1) * 31LL) % m;
	hash_value = (hash_value + hash_type(u + 1) * 961LL) % m;
	hash_value = (hash_value + hash_type(t + 1) * 12121LL) % m;

	return hash_value;
}

void parseData( MFnMesh & fnmesh, wr::MeshData<wr::Vertex>& mesh_data )
{
	int duplicate_count = 0;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	MPointArray mesh_points;
	fnmesh.getPoints(mesh_points, MSpace::kObject );

	//  Cache normals for each vertex
	MFloatVectorArray  mesh_normals;

	// Normals are per-vertex per-face..
	// use MItMeshPolygon::normalIndex() for index
	fnmesh.getNormals(mesh_normals);

	// Get UVSets for this mesh
	MStringArray  uv_sets;
	MStatus status = fnmesh.getUVSetNames(uv_sets);

	// Get all UV coordinates for the first UV set (default "map1").
	MFloatArray   u, v;
	fnmesh.getUVs( u, v, &uv_sets[0] );

	MFloatVectorArray  mesh_tangents;
	fnmesh.getTangents(mesh_tangents);

	MFloatVectorArray  mesh_binormals;
	fnmesh.getBinormals(mesh_binormals);

	mesh_data.m_vertices.reserve(mesh_points.length());
	mesh_data.m_indices = std::make_optional( std::vector<uint32_t>() );
	mesh_data.m_indices->reserve(mesh_points.length());

	MItMeshPolygon itt( fnmesh.object(), &status );

	std::vector<hash_type> hashes;
	hashes.reserve(mesh_points.length());

	uint32_t normal_index[3] = {0, 0, 0};
	uint32_t tangent_index[3] = {0, 0, 0};
	uint32_t hash_index[3] = {0, 0, 0};

	while (!itt.isDone())
	{
		// Get object-relative indices for the vertices in this face.
		MIntArray polygon_vertices;
		itt.getVertices(polygon_vertices);

		// Get triangulation of this polygon
		int num_triangles;
		itt.numTriangles(num_triangles);

		// needed for function but never used.
		MPointArray non_tweaked;

		// object-relative vertex indices for each triangle
		MIntArray triangle_vertices;

		// face-relative vertex indices for each triangle
		MIntArray localIndex;

		for (size_t i = 0; i < num_triangles; i++)
		{
			status = itt.getTriangles(non_tweaked,
				triangle_vertices,
				MSpace::kObject);

			if (status == MS::kSuccess)
			{
				MIntArray first_triangle_vertices;

				first_triangle_vertices.setLength(3);

				first_triangle_vertices[0] = triangle_vertices[0 + 3 * i];
				first_triangle_vertices[1] = triangle_vertices[1 + 3 * i];
				first_triangle_vertices[2] = triangle_vertices[2 + 3 * i];

				localIndex = GetLocalIndex(polygon_vertices, first_triangle_vertices);

				normal_index[0] = itt.normalIndex(localIndex[0]);
				normal_index[1] = itt.normalIndex(localIndex[1]);
				normal_index[2] = itt.normalIndex(localIndex[2]);

				tangent_index[0] = itt.tangentIndex(localIndex[0]);
				tangent_index[1] = itt.tangentIndex(localIndex[1]);
				tangent_index[2] = itt.tangentIndex(localIndex[2]);

				// Get Texture coordinates
				int first_uv_id[3];
				// Get UV values for each vertex within this polygon
				for (int vtx_in_polygon = 0; vtx_in_polygon < 3; vtx_in_polygon++)
				{
					itt.getUVIndex(localIndex[vtx_in_polygon],
						first_uv_id[vtx_in_polygon],
						&uv_sets[0]);
				}

				hash_type temp_hashes[3] = {
					compute_hash(first_triangle_vertices[0], normal_index[0], first_uv_id[0], tangent_index[0]),
					compute_hash(first_triangle_vertices[1], normal_index[1], first_uv_id[1], tangent_index[1]),
					compute_hash(first_triangle_vertices[2], normal_index[2], first_uv_id[2], tangent_index[2])
				};

				const char hash_res = checkVertexHash(temp_hashes, hashes, hash_index);

				wr::Vertex vertex;

				// Check if hash is found (if vertex is a duplicate)
				for (int j = 2; j >= 0; --j) {
					if (hash_res & (1 << j)) {
						++duplicate_count;
						mesh_data.m_indices->push_back(hash_index[j]);
					}
					else {
						vertex.m_pos[0] = (float)mesh_points[first_triangle_vertices[j]].x;
						vertex.m_pos[1] = (float)mesh_points[first_triangle_vertices[j]].y;
						vertex.m_pos[2] = (float)mesh_points[first_triangle_vertices[j]].z;
					
						vertex.m_normal[0] = mesh_normals[normal_index[j]].x;
						vertex.m_normal[1] = mesh_normals[normal_index[j]].y;
						vertex.m_normal[2] = mesh_normals[normal_index[j]].z;
					
						vertex.m_tangent[0] = mesh_tangents[tangent_index[j]].x;
						vertex.m_tangent[1] = mesh_tangents[tangent_index[j]].y;
						vertex.m_tangent[2] = mesh_tangents[tangent_index[j]].z;
					
						vertex.m_bitangent[0] = mesh_binormals[normal_index[j]].x;
						vertex.m_bitangent[1] = mesh_binormals[normal_index[j]].y;
						vertex.m_bitangent[2] = mesh_binormals[normal_index[j]].z;
					
						vertex.m_uv[0] = u[first_uv_id[j]];
						vertex.m_uv[1] = v[first_uv_id[j]];

						mesh_data.m_vertices.push_back(vertex);
						mesh_data.m_indices->push_back(mesh_data.m_vertices.size() - 1);
						hashes.push_back(temp_hashes[j]);
					}
				}
			}
			else
			{
				LOGC("Could not get the triangles.");
			}

		}
		itt.next();
	} // !itt.isDone()
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	MString time_str = std::to_string(time_span.count()).c_str();
	MGlobal::displayInfo(MString("Time in seconds: ") + time_str);
	MGlobal::displayInfo(MString("Number of duplicate vertices: ") + std::to_string(duplicate_count).c_str() + MString("/") + std::to_string(mesh_data.m_vertices.size() + duplicate_count).c_str());

	hashes.clear();
}
#pragma endregion

#pragma region callbacks
namespace wmr
{
	void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data )
	{
		// Check if attribute was set
		if( !( msg & MNodeMessage::kAttributeSet ) &&
			!( msg & MNodeMessage::kAttributeEval) )
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

		wmr::ModelParser* model_parser = reinterpret_cast<wmr::ModelParser*>(client_data);

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
			LOGW("Mesh added callback vector size is zero.");
			return;
		}

		auto it_end = --model_parser->m_mesh_added_callback_vector.end();

		if( it == it_end )
		{
			if (!findCallback(*it))
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
		if( !( msg & MNodeMessage::kAttributeSet ) &&
			!( msg & MNodeMessage::kAttributeEval ))
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

	// Check if the mesh is already added
	MObject mesh_object = fnmesh.object();
	auto itt = std::find_if(m_object_transform_vector.begin(), m_object_transform_vector.end(), getMeshObjectAlgorithm(mesh_object));
	if (itt != m_object_transform_vector.end())
	{
		// If it is, still add it to mesh changed vector
		auto changed_itt = std::find(m_changed_mesh_vector.begin(), m_changed_mesh_vector.end(), mesh_object);
		if (changed_itt == m_changed_mesh_vector.end())
		{
			m_changed_mesh_vector.push_back(mesh_object);
		}
		
		// If the model is already in the vector, assume that it should be overwritten
		// First remove it, then replace it
		m_renderer.GetScenegraph().DestroyNode<wr::MeshNode>(itt->second);
		itt->second = model_node;
	}
	else {
		m_object_transform_vector.push_back(std::make_pair(mesh_object, model_node));
	}

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

	LOG("Mesh \"{}\" added.", fnmesh.fullPathName().asChar());
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
			continue; // find_if returns last element even if it is not a positive result
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

void wmr::ModelParser::ToggleMeshVisibility(MPlug & plug_mesh, bool hide)
{
	// Check if the given plug is indeed a mesh object
	MStatus status;
	MFnMesh mesh = MFnMesh(plug_mesh.node(), &status);
	if (status != MS::kSuccess) {
		if (hide) {
			LOGW("Couldn't hide mesh, as the given plug isn't a mesh!");
		}
		else {
			LOGW("Couldn't show mesh, as the given plug isn't a mesh!");
		}
		return;
	}

	MObject mesh_object = mesh.object();

	// Get the itt of the given mesh object (from the standard meshes array)
	auto itt_mesh = std::find_if(m_object_transform_vector.begin(), m_object_transform_vector.end(), getMeshObjectAlgorithm(mesh_object));
	if (itt_mesh == m_object_transform_vector.end()) {
		if (hide) {
			LOG("Can't find the mesh to hide!");
		}
		else {
			LOG("Can't find the mesh to show!");
		}
		return;
	}

	// Hide/show the model
	itt_mesh->second->m_visible = !hide;
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
