#include "RendererMain.hpp"

#include "wisp.hpp"
#include "render_tasks/d3d12_test_render_task.hpp"
#include "render_tasks/d3d12_imgui_render_task.hpp"
#include "render_tasks/d3d12_deferred_main.hpp"
#include "render_tasks/d3d12_deferred_composition.hpp"
#include "frame_graph/frame_graph.hpp"
#include "scene_graph/scene_graph.hpp"

#include <algorithm>

bool main_menu = true;
bool open0 = true;
bool open1 = true;
bool open2 = true;
bool open_console = false;
bool show_imgui = true;
char message_buffer[600];

static wr::imgui::special::DebugConsole debug_console;

void RenderEditor()
{
	debug_console.Draw("Console", &open_console);

	if (!show_imgui) return;

	if (main_menu && ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit", "ALT+F4")) std::exit(0);
			if (ImGui::MenuItem("Hide ImGui", "F1")) show_imgui = false;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			wr::imgui::menu::Registries();
			ImGui::Separator();
			ImGui::MenuItem("Theme", nullptr, &open0);
			ImGui::MenuItem("ImGui Details", nullptr, &open1);
			ImGui::MenuItem("Logging Example", nullptr, &open2);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::DockSpaceOverViewport(main_menu, nullptr, ImGuiDockNodeFlags_PassthruDockspace);

	auto& io = ImGui::GetIO();

	// Create dockable background
	if (open0)
	{
		ImGui::Begin("Theme", &open0);
		if (ImGui::Button("Cherry")) ImGui::StyleColorsCherry();
		if (ImGui::Button("Unreal Engine")) ImGui::StyleColorsUE();
		if (ImGui::Button("Light Green")) ImGui::StyleColorsLightGreen();
		if (ImGui::Button("Light")) ImGui::StyleColorsLight();
		if (ImGui::Button("Dark")) ImGui::StyleColorsDark();
		if (ImGui::Button("Dark2")) ImGui::StyleColorsDarkCodz1();
		ImGui::End();
	}

	if (open1)
	{
		ImGui::Begin("ImGui Details", &open1);
		ImGui::Text("Mouse Pos: (%0.f, %0.f)", io.MousePos.x, io.MousePos.y);
		ImGui::Text("Framerate: %.0f", io.Framerate);
		ImGui::Text("Delta: %f", io.DeltaTime);
		ImGui::Text("Display Size: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y);
		ImGui::End();
	}

	if (open2)
	{
		ImGui::Begin("Logging Example", &open2);
		ImGui::InputText("Message", message_buffer, 600);
		if (ImGui::Button("LOG (Message)")) LOG(message_buffer);
		if (ImGui::Button("LOGW (Warning)")) LOGW(message_buffer);
		if (ImGui::Button("LOGE (Error)")) LOGE(message_buffer);
		if (ImGui::Button("LOGC (Critical)")) LOGC(message_buffer);
		ImGui::End();
	}

	wr::imgui::window::ShaderRegistry();
	wr::imgui::window::PipelineRegistry();
	wr::imgui::window::RootSignatureRegistry();
	wr::imgui::window::D3D12Settings();
}

wmr::wri::RendererMain::RendererMain()
{
}

wmr::wri::RendererMain::~RendererMain()
{
}

void wmr::wri::RendererMain::StartWispRenderer()
{
	// ImGui Logging
	util::log_callback::impl = [&](std::string const & str)
	{
		debug_console.AddLog(str.c_str());
	};

	m_render_system = std::make_unique<wr::D3D12RenderSystem>();

	// TODO: Add support for "windowless" rendering
	//m_render_system->Init(nullptr);

	m_scene_graph = std::make_unique<wr::SceneGraph>(m_render_system.get());
	m_frame_graph = std::make_unique<wr::FrameGraph>();

//#pragma region Cube sample
//	// Load custom model
//	auto model_pool = m_render_system->CreateModelPool(1, 1);
//	wr::Model* model;
//	{
//		wr::MeshData<wr::Vertex> mesh;
//		static const constexpr float size = 0.5f;
//
//		mesh.m_indices = {
//			2, 1, 0, 3, 2, 0, 6, 5,
//			4, 7, 6, 4, 10, 9, 8, 11,
//			10, 8, 14, 13, 12, 15, 14, 12,
//			18, 17, 16, 19, 18, 16, 22, 21,
//			20, 23, 22, 20
//		};
//
//		mesh.m_vertices = {
//			{ 1, 1, -1, 1, 1, 0, 0, -1 },
//			{ 1, -1, -1, 0, 1, 0, 0, -1 },
//			{ -1, -1, -1, 0, 0, 0, 0, -1 },
//			{ -1, 1, -1, 1, 0, 0, 0, -1 },
//			{ 1, 1, 1, 1, 1, 0, 0, 1 },
//			{ -1, 1, 1, 0, 1, 0, 0, 1 },
//			{ -1, -1, 1, 0, 0, 0, 0, 1 },
//			{ 1, -1, 1, 1, 0, 0, 0, 1 },
//			{ 1, 1, -1, 1, 0, 1, 0, 0 },
//			{ 1, 1, 1, 1, 1, 1, 0, 0 },
//			{ 1, -1, 1, 0, 1, 1, 0, 0 },
//			{ 1, -1, -1, 0, 0, 1, 0, 0 },
//			{ 1, -1, -1, 1, 0, 0, -1, 0 },
//			{ 1, -1, 1, 1, 1, 0, -1, 0 },
//			{ -1, -1, 1, 0, 1, 0, -1, 0 },
//			{ -1, -1, -1, 0, 0, 0, -1, 0 },
//			{ -1, -1, -1, 0, 1, -1, 0, 0 },
//			{ -1, -1, 1, 0, 0, -1, 0, 0 },
//			{ -1, 1, 1, 1, 0, -1, 0, 0 },
//			{ -1, 1, -1, 1, 1, -1, 0, 0 },
//			{ 1, 1, 1, 1, 0, 0, 1, 0 },
//			{ 1, 1, -1, 1, 1, 0, 1, 0 },
//			{ -1, 1, -1, 0, 1, 0, 1, 0 },
//			{ -1, 1, 1, 0, 0, 0, 1, 0 },
//		};
//
//		model = model_pool->LoadCustom<wr::Vertex>({ mesh });
//	}
//
//	auto mesh_node = scene_graph->CreateChild<wr::MeshNode>(nullptr, model);
//	auto camera = scene_graph->CreateChild<wr::CameraNode>(nullptr, 70.f, 1280.0f / 720.0f);
//
//	// #### background cubes
//	std::vector<std::pair<std::shared_ptr<wr::MeshNode>, int>> bg_nodes(500);
//	float distance = 20;
//	float cube_size = 2.5f;
//
//	float start_x = -30;
//	float start_y = -18;
//	float max_column_width = 30;
//
//	int column = 0;
//	int row = 0;
//
//	srand(time(0));
//	int rand_max = 15;
//
//	for (auto& node : bg_nodes)
//	{
//		node.first = scene_graph->CreateChild<wr::MeshNode>(nullptr, model);
//		node.first->SetPosition({ start_x + (cube_size * column), start_y + (cube_size * row), distance });
//		node.second = (rand() % rand_max + 0);
//
//		column++;
//		if (column > max_column_width) { column = 0; row++; }
//	}
//	// ### background cubes
//
//	camera->SetPosition(0, 0, -5);
//#pragma endregion

	m_render_system->InitSceneGraph(*m_scene_graph.get());

	m_frame_graph->AddTask(wr::GetDeferredMainTask());
	m_frame_graph->AddTask(wr::GetDeferredCompositionTask());
	m_frame_graph->AddTask(wr::GetImGuiTask(&RenderEditor));
	m_frame_graph->Setup(*m_render_system);
}

void wmr::wri::RendererMain::StopWispRenderer()
{
	m_render_system->WaitForAllPreviousWork();
	m_frame_graph->Destroy();
	m_render_system.reset();
}
