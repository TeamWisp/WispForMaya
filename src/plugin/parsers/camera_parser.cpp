#include "camera_parser.hpp"

// Wisp plug-in
#include "plugin/renderer/renderer.hpp"
#include "plugin/viewport_renderer_override.hpp"

// Wisp rendering framework
#include "scene_graph/camera_node.hpp"

// Maya API
#include <maya/M3dView.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnCamera.h>
#include <maya/MFnTransform.h>
#include <maya/MViewport2Renderer.h>

void wmr::CameraParser::Initialize()
{
	m_viewport_camera = dynamic_cast<const ViewportRendererOverride*>(MHWRender::MRenderer::theRenderer()->findRenderOverride(settings::VIEWPORT_OVERRIDE_NAME))->GetRenderer().GetCamera();
}

void wmr::CameraParser::UpdateViewportCamera(const MString & panel_name)
{
	M3dView viewport;

	// Try to retrieve the current active viewport panel
	auto status = M3dView::getM3dViewFromModelPanel(panel_name, viewport);

	// Could not retrieve the viewport panel
	if (status == MStatus::kFailure)
		return;

	// Model view matrix
	MMatrix model_view_matrix;
	viewport.modelViewMatrix(model_view_matrix);

	MDagPath camera_dag_path;
	viewport.getCamera(camera_dag_path);
	MFnTransform camera_transform(camera_dag_path.transform());

	MEulerRotation view_rotation;
	camera_transform.getRotation(view_rotation);

	m_viewport_camera->SetRotation({ (float)view_rotation.x,(float)view_rotation.y, (float)view_rotation.z });

	MMatrix cameraPos = camera_dag_path.inclusiveMatrix();
	MVector eye = MVector(static_cast<float>(cameraPos(3, 0)), static_cast<float>(cameraPos(3, 1)), static_cast<float>(cameraPos(3, 2)));
	m_viewport_camera->SetPosition({ (float)-eye.x, (float)-eye.y, (float)-eye.z });

	MFnCamera camera_functions(camera_dag_path);
	m_viewport_camera->m_frustum_far = camera_functions.farClippingPlane();
	m_viewport_camera->m_frustum_near = camera_functions.nearClippingPlane();

	m_viewport_camera->SetAspectRatio(1.0f);
	m_viewport_camera->SetFov(AI_RAD_TO_DEG(camera_functions.horizontalFieldOfView()));
}
