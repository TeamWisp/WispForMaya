#include "RendererMain.hpp"

#include <memory>
#include <algorithm>
#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"

// TODO: Remove this once a proper texture class has been implemented
#include "renderer.hpp"

#include "../demo/scene_viknell.hpp"
#include "../demo/resources.hpp"
#include "../demo/scene_cubes.hpp"

#define SCENE viknell_scene

wmr::wri::RendererMain::RendererMain()
{
}

wmr::wri::RendererMain::~RendererMain()
{
}

void wmr::wri::RendererMain::StartWispRenderer()
{
	t_render_system = std::make_unique<wr::D3D12RenderSystem>();

	t_render_system->Init(std::nullopt);

	resources::CreateResources(t_render_system.get());

	t_scene_graph = std::make_shared<wr::SceneGraph>(t_render_system.get());

	// TODO: Remove these hard-coded values
	SCENE::CreateScene(t_scene_graph.get(), 1280.0f / 720.0f);

	t_render_system->InitSceneGraph(*t_scene_graph.get());

	t_frame_graph = std::make_unique<wr::FrameGraph>();
	t_frame_graph->AddTask(wr::GetDeferredMainTask(1280, 720));
	t_frame_graph->AddTask(wr::GetDeferredCompositionTask(1280, 720));
	t_frame_graph->Setup(*t_render_system);
}

wr::Texture* wmr::wri::RendererMain::UpdateWispRenderer()
{
	SCENE::UpdateScene();

	return t_render_system->Render(t_scene_graph, *t_frame_graph).get();
}

void wmr::wri::RendererMain::StopWispRenderer()
{
	t_render_system->WaitForAllPreviousWork(); // Make sure GPU is finished before destruction.
	t_frame_graph->Destroy();
	t_render_system.reset();
}
