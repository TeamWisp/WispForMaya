#pragma once

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain() = default;
		~RendererMain() = default;

		void Initialize();
		void Update();
		void Cleanup();

	private:
	};
}