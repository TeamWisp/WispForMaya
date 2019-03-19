#pragma once
#include <maya/MApiNamespace.h>
#include <maya/MNodeMessage.h>

#include <vector>
#include <memory>


namespace wr
{
	struct MeshNode;
}

namespace wmr
{
	class Renderer;
	class ModelParser
	{
		//friend callbacks
		friend void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		
	public:
		ModelParser();
		~ModelParser();

		void SubscribeObject( MObject& maya_object );
		void UnSubscribeObject( MObject& maya_object );
		void MeshAdded( MFnMesh & fnmesh );
		std::shared_ptr<wr::MeshNode> GetWRModel(MObject & maya_object);

	private:
		//callbacks that require private access and are part of the ModelParser.
		friend void AttributeMeshAddedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		friend void MeshRemovedCallback( MObject& node, void* client_data );

		std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;

		Renderer& m_renderer;
	};
}
