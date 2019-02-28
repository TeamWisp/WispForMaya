#pragma once

// Maya API
#include <maya/MApiNamespace.h>

// C++ standard
#include <memory>

namespace wr
{
	class CameraNode;
}

namespace wmr
{
	class CameraParser
	{
	public:
		CameraParser() = default;
		~CameraParser() = default;

		//! Fetches a reference to the renderer
		void Initialize();

		//! Set the Wisp camera to the currently active Maya camera
		void UpdateViewportCamera(const MString& panel_name);

	private:
		//! Pointer to the Wisp camera used to render the scene
		std::shared_ptr<wr::CameraNode> m_viewport_camera;
	};
}