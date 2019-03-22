#pragma once
#include <maya/MApiNamespace.h>
#include <maya/MNodeMessage.h>
#include <scene_graph/scene_graph.hpp>	
#include <vector>
#include <memory>


namespace wr
{
	class LightNode;
}



namespace wmr
{
	class Renderer;
	class LightParser
	{
		//friend callbacks
		
	public:
		LightParser();
		~LightParser();

		void SubscribeObject( MObject& maya_object );
		void UnSubscribeObject( MObject& maya_object );
		void LightAdded( MFnLight & fn_light );

	private:
		//callbacks that require private access and are part of the ModelParser.
		friend void AttributeLightTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );

		std::vector<std::pair<MObject, std::shared_ptr<wr::LightNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;

		Renderer& m_renderer;
	};

	
}
