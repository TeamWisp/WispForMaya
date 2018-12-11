#include "Renderer.hpp"

#include "miscellaneous/Functions.hpp"

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"

#include "../demo/engine_interface.hpp"
#include "../demo/scene_viknell.hpp"
#include "../demo/resources.hpp"
#include "../demo/scene_cubes.hpp"

#define SCENE viknell_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;

wr::FrameGraph* fg_ptr = nullptr;

auto window = std::make_unique<wr::Window>(
	GetModuleHandleA(nullptr),
	"Wisp Ray-traced Viewport Renderer",
	1280,
	720);

void RenderEditor()
{
	engine::RenderEngine(render_system.get(), scene_graph.get());
}

namespace wmr::wri
{
	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}
	
	void Renderer::Initialize(unsigned int render_target_width, unsigned int render_target_height)
	{
		// ImGui Logging
		util::log_callback::impl = [&](std::string const & str)
		{
			engine::debug_console.AddLog(str.c_str());
		};

		render_system = std::make_unique<wr::D3D12RenderSystem>();

		window->SetKeyCallback([](int key, int action, int mods)
		{
			if (action == WM_KEYUP && key == 0xC0)
			{
				engine::open_console = !engine::open_console;
				engine::debug_console.EmptyInput();
			}
			if (action == WM_KEYUP && key == VK_F1)
			{
				engine::show_imgui = !engine::show_imgui;
			}
		});

		render_system->Init(window.get());

		resources::CreateResources(render_system.get());

		scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

		SCENE::CreateScene(scene_graph.get(), window.get());

		render_system->InitSceneGraph(*scene_graph.get());

		fg_ptr = new wr::FrameGraph();
		fg_ptr->AddTask(wr::GetDeferredMainTask());
		fg_ptr->AddTask(wr::GetDeferredCompositionTask());
		fg_ptr->AddTask(wr::GetImGuiTask(&RenderEditor));
		fg_ptr->Setup(*render_system);
	}
	
	void Renderer::Update()
	{
		window->PollEvents();

		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, *fg_ptr);
	}
	
	void Renderer::Destroy()
	{
		// Make sure the GPU has finished executing the final command list before starting the cleanup
		render_system->WaitForAllPreviousWork();
		fg_ptr->Destroy();
		delete fg_ptr;
		render_system.reset();
	}
}