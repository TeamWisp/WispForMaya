#pragma once

#include "imgui_tools.hpp"

#include <memory>

namespace wmr::wri
{
	class RendererMain
	{
	public:
		RendererMain();
		~RendererMain();

		void StartWispRenderer();

	private:
		void RenderEditor();

	private:
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;
		static wr::imgui::special::DebugConsole m_debug_console;

		// TODO: Remove these ImGui helper variables once a real ImGui
		// tool is in place, this is just to demonstrate ImGui running
		bool m_main_menu;
		bool m_open0;
		bool m_open1;
		bool m_open2;
		bool m_open_console;
		bool m_show_imgui;
		char message_buffer[600];
	};
}