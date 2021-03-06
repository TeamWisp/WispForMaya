// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "camera_parser.hpp"

// Wisp plug-in
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Wisp rendering framework
#include "scene_graph/camera_node.hpp"
#include "util/log.hpp"

// Maya API
#include <maya/M3dView.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnCamera.h>
#include <maya/MFnTransform.h>
#include <maya/MViewport2Renderer.h>

#include <DirectXMath.h>

void wmr::CameraParser::Initialize()
{
	LOG("Attempting to get a reference to the renderer.");
	m_viewport_camera = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetCamera();
}

void wmr::CameraParser::UpdateViewportCamera(const MString & panel_name)
{
	M3dView viewport;

	// Try to retrieve the current active viewport panel
	auto status = M3dView::getM3dViewFromModelPanel(panel_name, viewport);

	// Could not retrieve the viewport panel
	if (status == MStatus::kFailure)
	{
		LOGE("Could not retrieve the viewport panel.");
		return;
	}

	// Model view matrix
	MMatrix model_view_matrix;
	viewport.modelViewMatrix(model_view_matrix);

	MDagPath camera_dag_path;
	viewport.getCamera(camera_dag_path);
	MFnCamera camera_functions(camera_dag_path);

	// Ignore orthographic cameras
	if (camera_functions.isOrtho())
	{
		LOGE("User tried using an orthogonal camera, Wisp does not support this.");
		return;
	}

	MFnTransform camera_transform(camera_dag_path.transform());

	MEulerRotation view_rotation;
	camera_transform.getRotation(view_rotation);

	m_viewport_camera->SetRotation({ (float)view_rotation.x,(float)view_rotation.y, (float)view_rotation.z });

	MVector cameraPos = camera_functions.eyePoint(MSpace::kWorld);
	m_viewport_camera->SetPosition({ (float)cameraPos.x, (float)cameraPos.y, (float)cameraPos.z });

	// Position and dimensions of the current Maya viewport
	std::uint32_t x, y, current_viewport_width, current_viewport_height;
	status = viewport.viewport(x, y, current_viewport_width, current_viewport_height);

	// Could not retrieve the viewport information
	if (status == MStatus::kFailure)
	{
		LOGE("Could not retrieve viewport dimensions.");
		return;
	}

	m_viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
	m_viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();

	m_viewport_camera->m_enable_dof = camera_functions.isDepthOfField();
	m_viewport_camera->m_f_number = camera_functions.fStop();
	m_viewport_camera->m_focus_dist = camera_functions.focusDistance();
	m_viewport_camera->m_focal_length = camera_functions.focalLength();
	// Get DoF region scale
	// There is no function to get this value
	m_viewport_camera->m_dof_range = camera_functions.findPlug("focusRegionScale").asFloat();

	m_viewport_camera->SetFov(DirectX::XMConvertToDegrees(camera_functions.horizontalFieldOfView()));
	m_viewport_camera->SetAspectRatio((float)current_viewport_width / (float)current_viewport_height);

	MMatrix proj;
	viewport.projectionMatrix(proj);

	DirectX::XMVECTOR vec0 = DirectX::XMVectorSet(proj.matrix[0][0], proj.matrix[0][1], proj.matrix[0][2], proj.matrix[0][3]);
	DirectX::XMVECTOR vec1 = DirectX::XMVectorSet(proj.matrix[1][0], proj.matrix[1][1], proj.matrix[1][2], proj.matrix[1][3]);
	DirectX::XMVECTOR vec2 = DirectX::XMVectorSet(proj.matrix[2][0], proj.matrix[2][1], proj.matrix[2][2], proj.matrix[2][3]);
	DirectX::XMVECTOR vec3 = DirectX::XMVectorSet(proj.matrix[3][0], proj.matrix[3][1], proj.matrix[3][2], proj.matrix[3][3]);

	m_viewport_camera->m_projection = DirectX::XMMATRIX(vec0, vec1, vec2, vec3);
}
