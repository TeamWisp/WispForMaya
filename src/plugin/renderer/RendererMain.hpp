#pragma once

namespace wr
{
	class D3D12RenderSystem;
	class SceneGraph;
}

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain() = default;
		~RendererMain() = default;

		void Initialize();
		void Update();
		void Resize(unsigned int new_width, unsigned int new_height);
		void Cleanup();
	};
}