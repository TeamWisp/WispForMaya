#include "Renderer.hpp"

#include "miscellaneous/Settings.hpp"
#include "miscellaneous/Functions.hpp"

#include <memory>
#include <algorithm>
#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "render_tasks/d3d12_deferred_render_target_copy.hpp"

#include "../demo/engine_interface.hpp"
#include "../demo/scene_viknell.hpp"
#include "../demo/resources.hpp"
#include "../demo/scene_cubes.hpp"

#include <maya/MString.h>
#include <maya/M3dView.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>

#define SCENE viknell_scene

std::unique_ptr<wr::D3D12RenderSystem> render_system;
std::shared_ptr<wr::SceneGraph> scene_graph;
std::shared_ptr<wr::CameraNode> viewport_camera;

std::shared_ptr<wr::TexturePool> texture_pool;

wr::FrameGraph* fg_ptr = nullptr;
auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

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

		render_system->Init(window.get());

		resources::CreateResources(render_system.get());

		scene_graph = std::make_shared<wr::SceneGraph>(render_system.get());

		viewport_camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 90.f, (float)window->GetWidth() / (float)window->GetHeight());
		viewport_camera->SetPosition({ 0, 0, -1 });

		SCENE::CreateScene(scene_graph.get(), window.get());

		render_system->InitSceneGraph(*scene_graph.get());

		fg_ptr = new wr::FrameGraph();
		fg_ptr->AddTask(wr::GetDeferredMainTask());
		fg_ptr->AddTask(wr::GetDeferredCompositionTask());
		fg_ptr->AddTask(wr::GetRenderTargetCopyTask<wr::DeferredCompositionTaskData>());
		fg_ptr->AddTask(wr::GetImGuiTask(&RenderEditor));
		fg_ptr->Setup(*render_system);
	}
	
	void Renderer::Update()
	{
		SynchronizeWispWithMayaViewportCamera();
		SCENE::UpdateScene();

		auto texture = render_system->Render(scene_graph, *fg_ptr);
	}
	
	void Renderer::Destroy()
	{
		// Make sure the GPU has finished executing the final command list before starting the cleanup
		render_system->WaitForAllPreviousWork();
		fg_ptr->Destroy();
		render_system.reset();

		delete fg_ptr;
	}

	void Renderer::SynchronizeWispWithMayaViewportCamera()
	{
		M3dView view;
		MStatus status = M3dView::getM3dViewFromModelPanel(wisp::settings::VIEWPORT_PANEL_NAME, view);

		if (status != MStatus::kSuccess)
		{
			// Failure means no camera data for this frame, early-out!
			return;
		}

		// Model view matrix
		MMatrix mv_matrix;
		view.modelViewMatrix(mv_matrix);

		MDagPath camera_dag_path;
		view.getCamera(camera_dag_path);

		// Additional functionality
		MFnCamera camera_functions(camera_dag_path);

		MVector center = camera_functions.centerOfInterestPoint(MSpace::kWorld);
		MVector eye = camera_functions.eyePoint(MSpace::kWorld);

		viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
		viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();

		viewport_camera->SetFov(camera_functions.horizontalFieldOfView());

		// Convert the MMatrix into an XMMATRIX and update the view matrix of the Wisp camera
		DirectX::XMFLOAT4X4 converted_mv_matrix;
		mv_matrix.get(converted_mv_matrix.m);
		viewport_camera->m_view = DirectX::XMLoadFloat4x4(&converted_mv_matrix);

		viewport_camera->SetPosition({ (float)-eye.x, (float)eye.y, (float)-eye.z });
	}
}