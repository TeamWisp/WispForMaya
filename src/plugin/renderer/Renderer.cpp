#include "Renderer.hpp"

#include "miscellaneous/Settings.hpp"
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

#include <maya/MString.h>
#include <maya/M3dView.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>

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

		m_viewport_camera = scene_graph->CreateChild<wr::CameraNode>(
			nullptr,
			90.f,
			(float)window->GetWidth() / (float)window->GetHeight());

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

		SynchronizeWispWithMayaViewportCamera();
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

	void Renderer::SynchronizeWispWithMayaViewportCamera()
	{
		M3dView view;
		MStatus status = M3dView::getM3dViewFromModelPanel(wisp::settings::VIEWPORT_PANEL_NAME, view);

		if (status != MStatus::kSuccess)
		{
			// Failure means no camera data for this frame, early-out!
			return;
		}

		MMatrix view_matrix;
		MMatrix transform_matrix;

		view.modelViewMatrix(view_matrix);

		MDagPath camera_dag_path;
		view.getCamera(camera_dag_path);

		// Additional functionality
		MFnCamera camera_functions(camera_dag_path);

		transform_matrix = camera_dag_path.inclusiveMatrix();

		// The camera matrix is column-major, so the bottom-row can be used to retrieve the position data
		DirectX::XMVECTOR position = DirectX::XMVectorSet(
			static_cast<float>(transform_matrix[3][0]),
			static_cast<float>(transform_matrix[3][1]),
			static_cast<float>(transform_matrix[3][2]),
			static_cast<float>(transform_matrix[3][3]));

		m_viewport_camera->SetPosition(position);

		m_viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
		m_viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();

		m_viewport_camera->SetFov(camera_functions.horizontalFieldOfView());

		// The matrix is manually transposed by passing it as row-major
		m_viewport_camera->m_view = DirectX::XMMatrixSet(
			view_matrix[0][0], view_matrix[1][0], view_matrix[2][0], view_matrix[3][0],
			view_matrix[0][1], view_matrix[1][1], view_matrix[2][1], view_matrix[3][1],
			view_matrix[0][2], view_matrix[1][2], view_matrix[2][2], view_matrix[3][2],
			view_matrix[0][3], view_matrix[1][3], view_matrix[2][3], view_matrix[3][3]);
	}
}