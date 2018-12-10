#pragma once

#include <memory>

namespace wmr::wri
{
	class Renderer;

	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void Initialize();
		void Update();
		void Resize(unsigned int new_width, unsigned int new_height);
		void Cleanup();

	private:
		std::unique_ptr<Renderer> m_renderer;
	};
}