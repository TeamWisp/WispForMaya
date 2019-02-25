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

		void MeshAdded( MFnMesh & fnmesh );

	private:
		std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>> m_object_transform_vector;
		Renderer& m_render_system;
	};
}
