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

#pragma once

// Wisp plug-in
#include <miscellaneous/settings.hpp>

// Maya API
#include <maya/MShaderManager.h>

// C++ standard
#include <array>
#include <memory>

//! Wisp rendering framework namespace (Wisp Renderer)
namespace wr
{
	// Forward declarations
	struct CameraNode;
	struct CPUTexture;
	struct CPUTextures;

	class AssimpModelLoader;
	class D3D12RenderSystem;
	class FrameGraph;
	class SceneGraph;
	class TexturePool;
}

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
    // Forward declarations
	class ScenegraphParser;
	class Renderer;

	//! Indicates which buffer should be used to save the Wisp renderer output data
	enum class WispBufferType
	{
		COLOR,	/*!< Store the data in the color buffer. */
		DEPTH	/*!< Store the data in the depth buffer.*/
	};

	//! Viewport renderer  override implementation
	/*! Implementation of a Maya MRenderOverride. It inherits from the Maya API base class and implements all methods
	 *  needed to make the override work. The code style for functions is a bit different here because our style guide
	 *  differs from the style used for the Maya API. */
	class ViewportRendererOverride final : public MHWRender::MRenderOverride
	{
	public:
		//! Prepares the override for initialization and initializes
		/*! When creating an instance of this class, the parent constructor is called, as well as two functions that will
		 *  prepare the member variables for the initialization function later on.
		 *  
		 *  /param name The name of this renderer override.
		 *  /sa ConfigureRenderOperations()
		 *  /sa SetDefaultTextureState()
		 *  /sa InitializeWispRenderer()*/
		ViewportRendererOverride(const MString& name);

		//! Stop the Wisp renderer and release any Maya textures
		/*! When the override is destroyed, the Wisp renderer has to be killed first. To avoid nasty errors, this function
		 *  first stops the Wisp renderer (synchronizes CPU and GPU) before removing its model loader object, render system,
		 *  and frame graph manager. Afterwards, all remaining Maya textures will be released.
		 *
		 *  /sa ReleaseTextureResources()*/
		~ViewportRendererOverride() override;

		//! Release resources, but don't deregister the override yet!
		void Destroy() noexcept;

		//! Returns the name of the plug-in that should show up under the "renderer" drop-down menu in the Maya viewport
		/*! /return Name of the drop-down menu item. */
		MString uiName() const override;

		//! Get hold of the renderer
		/*! /return Renderer reference. */
		Renderer& GetRenderer() const;

		//! Get hold of the scene graph parser
		/*! /return SceneGraphParser reference. */
		wmr::ScenegraphParser& GetSceneGraphParser() const;

		//! Get the viewport width and height
		const std::pair<uint32_t, uint32_t> GetViewportSize() const noexcept;

		//! Lets the caller know when the plug-in has completed at least a single setup loop
		bool IsInitialized() const noexcept;

	private:
		//! Assign the correct render operations to the render operation container
		/*! The names specified by the ConfigureRenderOperations() function indicate the order in which the render
		 *  operations are expected.
		 *  
		 *  /sa ScreenRenderOperation
		 *  /sa GizmoRenderOperation */
		void CreateRenderOperations();

		//! Indicate which rendering back-ends are supported by the plug-in
		/*! The Maya API requires us to override this function. The return value of the function is a flag that indicates
		 *  whether OpenGL, OpenGL Core, or DX11 is supported.
		 *  
		 *  /return Flag that indicates which rendering APIs are supported. */
		// Which Maya rendering back ends are supported by this plug-in?
		MHWRender::DrawAPI supportedDrawAPIs() const override;

		//! Loop through all render operations and return the current active operation to Maya
		/*! Simple implementation of the Maya render operation loop as seen in the SDK samples. For more information,
		 *  please refer to the Autodesk documentation or the SDK samples.
		 *  
		 *  /return Current render operation. */
		MHWRender::MRenderOperation* renderOperation() override;
		
		//! Update function, called whenever the viewport needs to be rendered / refreshed
		/*! This is where the most important code happens, as this is where the Wisp rendering framework render loop is
		 *  implemented.
		 *  
		 *  /return Returns kSuccess upon successful execution. */
		MStatus setup(const MString& destination) override;

		//! Updates application state when viewport has been resized
		/*! /param panel_name The name of the current viewport panel function. */
		void HandleViewportResize(const MString& panel_name) noexcept;

		//! A simple check that checks whether all render operations are valid (no nullptr)
		/*! /return True if everything is correct, else, false. */
		bool AreAllRenderOperationsSetCorrectly() const;

		//! Clean-up the viewport override
		/*! Sets the current render operation to -1 and always returns kSuccess.
		 *
		 *  /return kSuccess is always returned from this function, it simply cannot fail. */
		MStatus cleanup() override;

		//! Sets the current render operation to 0
		/*! /return Always returns true no matter what. This function cannot fail. */
		bool startOperationIterator() override;

		//! Move to the next render operation
		/*! Advance the current render operation index by one. Maya will reset the render operation index as soon as this
		 *  function returns true (indicating that the final operation has finished executing).
		 *  
		 *  /return True when the last operation has been selected, else, false. */
		bool nextRenderOperation() override;

	private:
		void InitialNotifyUser();
		
		MString m_ui_name; //!< Name of the ui panel that will be overridden

		std::array<std::unique_ptr<MHWRender::MRenderOperation>, settings::RENDER_OPERATION_COUNT> m_render_operations; //!< All render operations used in this plug-in		

		int m_current_render_operation; //!< Index of the currently active render operation

		std::unique_ptr<Renderer> m_renderer; //!< Wisp framework render system
		std::unique_ptr<wmr::ScenegraphParser> m_scenegraph_parser;

		uint32_t m_viewport_width;
		uint32_t m_viewport_height;

		bool m_is_initialized;
	};
}