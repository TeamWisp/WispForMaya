// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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
		friend void AttributeLightCallback( MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData );

		std::vector<std::pair<MObject, std::shared_ptr<wr::LightNode>>> m_object_transform_vector;
		std::vector<std::pair<MObject, MCallbackId>> m_mesh_added_callback_vector;

		Renderer& m_renderer;
	};
}
