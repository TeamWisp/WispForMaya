#pragma once

#include <memory>

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void StartWispRenderer();
		void UpdateWispRenderer();
		void StopWispRenderer();

	private:
	};
}