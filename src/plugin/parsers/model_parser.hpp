#pragma once
#include <maya/MApiNamespace.h>
#include <maya/MNodeMessage.h>

#include <vector>
#include <memory>

#include <functional>

namespace wr
{
	struct MeshNode;
}

namespace wmr
{
	class Renderer;
	class ModelParser
	{
		
		
	public:
		ModelParser();
		~ModelParser();

		void SubscribeObject( MObject& maya_object );
		void UnSubscribeObject( MObject& maya_object );
		void MeshAdded( MFnMesh & fnmesh );
		std::shared_ptr<wr::MeshNode> GetWRModel(MObject & maya_object);

		void Update();

		void SetMeshAddCallback(std::function<void(MFnMesh&)> callback);

		// Show/Hide meshes
		void ShowMesh(MPlug & plug_mesh);
		void HideMesh(MPlug & plug_mesh);

	private:
		//callbacks that require private access and are part of the ModelParser.
		friend void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		friend void AttributeMeshAddedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		friend void attributeMeshChangedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data );


		std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;
		std::vector<MObject> m_changed_mesh_vector;
		// Vector of the same type as m_object_transform_vector taht contains hidden meshes
		decltype(m_object_transform_vector) m_hidden_meshes;

		Renderer& m_renderer;

		std::function<void(MFnMesh&)> mesh_add_callback;
	};
}
