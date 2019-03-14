#pragma once
#include <maya/MApiNamespace.h>
#include <maya/MNodeMessage.h>

#include <vector>
#include <memory>


namespace wr
{
	class MeshNode;
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
		void MeshAdded( MFnMesh & fnmesh );

	private:
		friend void AttributeMeshAddedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );

		std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;

		Renderer& m_renderer;
	};
}
