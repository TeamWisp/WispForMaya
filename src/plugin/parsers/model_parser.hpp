// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zijnstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
		void ToggleMeshVisibility(MPlug & plug_mesh, bool hide);

	private:
		//callbacks that require private access and are part of the ModelParser.
		friend void AttributeMeshTransformCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		friend void AttributeMeshAddedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );
		friend void attributeMeshChangedCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &other_plug, void *client_data );

		std::vector<std::pair<MObject, std::shared_ptr<wr::MeshNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;
		std::vector<MObject> m_changed_mesh_vector;

		Renderer& m_renderer;

		std::function<void(MFnMesh&)> mesh_add_callback;
	};
}
