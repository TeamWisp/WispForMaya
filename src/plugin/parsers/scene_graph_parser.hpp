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

// Maya API
#include <maya/MApiNamespace.h>
#include <maya/MMessage.h>

#include <memory>

namespace wr
{
	class SceneGraph;
	class D3D12RenderSystem;
	class TexturePool;
	class MaterialPool;
}

namespace wmr
{
	class CameraParser;
	class LightParser;
	class MaterialParser;
	class ModelParser;
	class Renderer;
	class ViewportRendererOverride;
	
	class ScenegraphParser
	{
	public:
		ScenegraphParser();
		~ScenegraphParser();

		void Initialize();
		void Update();

		void AddCallbackValidation(MStatus status, MCallbackId id);

		ModelParser& GetModelParser() const noexcept;
		MaterialParser& GetMaterialParser() const noexcept;
		CameraParser& GetCameraParser() const noexcept;
		LightParser& GetLightParser() const noexcept;

	private:
		Renderer& m_render_system;
		std::unique_ptr<LightParser> m_light_parser;
		std::unique_ptr<ModelParser> m_model_parser;
		std::unique_ptr<CameraParser> m_camera_parser;
		std::unique_ptr<MaterialParser> m_material_parser;

	};
}
