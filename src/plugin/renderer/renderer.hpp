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
#include <memory>

#include "frame_graph/frame_graph.hpp"

namespace wr
{
	class D3D12RenderSystem;
	class Window;
	class SceneGraph;
	class CameraNode;
}

namespace wmr
{

	class ModelManager;
	class MaterialManager;
	class TextureManager;
	class FrameGraphManager;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		//! Initialize all renderer systems
		void Initialize() noexcept;

		//! Update the frame index
		void Update();

		//! Request the Wisp renderer to render a frame
		void Render();

		//! Destroy all resources allocated by the renderer
		void Destroy();

		//! Update the skybox using the new path
		void UpdateSkybox(const std::string& path) noexcept;
		
		//! CPU texture data of the final render output and its depth buffer
		const wr::CPUTextures GetRenderResult();

		//! Frame index tracking for internal rendering operations
		const std::uint64_t GetFrameIndex() const noexcept(true);

		//! Get a reference to the model manager
		ModelManager& GetModelManager() const;

		//! Get a reference to the frame graph
		FrameGraphManager& GetFrameGraph() const;

		//! Get a reference to the material manager
		MaterialManager& GetMaterialManager() const;

		//! Get a reference to the texture manager
		TextureManager& GetTextureManager() const;

		//! Get a reference to the scene graph
		wr::SceneGraph& GetScenegraph() const;

		//! Get a reference to the internal DX12 renderer
		wr::D3D12RenderSystem& GetD3D12Renderer() const;

		//! Get the viewport camera object as a shared pointer
		std::shared_ptr<wr::CameraNode> GetCamera() const;

	private:
		std::unique_ptr<FrameGraphManager>		m_framegraph_manager;
		std::unique_ptr<MaterialManager>		m_material_manager;
		std::unique_ptr<ModelManager>			m_model_manager;
		std::unique_ptr<TextureManager>			m_texture_manager;

		std::unique_ptr<wr::D3D12RenderSystem>	m_render_system;
		std::shared_ptr<wr::SceneGraph>			m_scenegraph;
		std::unique_ptr<wr::Window>				m_window;
		std::shared_ptr<wr::CameraNode>			m_wisp_camera;

		wr::CPUTextures							m_result_textures;

		std::uint64_t m_frame_index;
	};
}
