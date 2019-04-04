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
