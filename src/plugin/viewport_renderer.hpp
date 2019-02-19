#pragma once

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

	//! Indicates which buffer should be used to save the Wisp renderer output data
	enum class WispBufferType
	{
		COLOR,	/*!< Store the data in the color buffer. */
		DEPTH	/*!< Store the data in the depth buffer.*/
	};

	// Forward declaration
	class FrameGraphManager;

	//! Viewport renderer  override implementation
	/*! Implementation of a Maya MRenderOverride. It inherits from the Maya API base class and implements all methods
	 *  needed to make the override work. The code style for functions is a bit different here because our style guide
	 *  differs from the style used for the Maya API. */
	class ViewportRenderer final : public MHWRender::MRenderOverride
	{
	public:
		//! Prepares the override for initialization and initializes
		/*! When creating an instance of this class, the parent constructor is called, as well as two functions that will
		 *  prepare the member variables for the initialization function later on.
		 *  
		 *  \param name The name of this renderer override.
		 *  \sa ConfigureRenderOperations()
		 *  \sa SetDefaultTextureState()
		 *  \sa InitializeWispRenderer()*/
		ViewportRenderer(const MString& name);

		//! Stop the Wisp renderer and release any Maya textures
		/*! When the override is destroyed, the Wisp renderer has to be killed first. To avoid nasty errors, this function
		 *  first stops the Wisp renderer (synchronizes CPU and GPU) before removing its model loader object, render system,
		 *  and frame graph manager. Afterwards, all remaining Maya textures will be released.
		 *
		 *  \sa ReleaseTextureResources()*/
		~ViewportRenderer() override;

	private:
		//! Set the names of the render operations
		void ConfigureRenderOperations();

		//! Create an empty texture and its description
		/*! When the texture object is created, its texture resource is set to nullptr. The texture description is set
		 *  to the default 2D texture description values. */
		void SetDefaultTextureState();

		//! Release allocated textures
		/*! When this function is called, all textures that were in use by the plug-in will be released and cleaned-up by
		 *  Maya itself.*/
		void ReleaseTextureResources() const;
		
		//! Assign the correct render operations to the render operation container
		/*! The names specified by the ConfigureRenderOperations() function indicate the order in which the render
		 *  operations are expected.
		 *  
		 *  \sa WispScreenBlitter
		 *  \sa WispUIRenderer */
		void CreateRenderOperations();

		//! Initializes the Wisp rendering framework
		/*! To use the Wisp rendering framework, a couple things have to be initialized. This is determined by the framework
		 *! API itself, so these things cannot be changed all that much. This function creates a render system, model loader,
		 *! scene graph, and a frame graph manager.
		 *! 
		 *! \sa FrameGraphManager*/
		void InitializeWispRenderer();

		//! Indicate which rendering back-ends are supported by the plug-in
		/*! The Maya API requires us to override this function. The return value of the function is a flag that indicates
		 *  whether OpenGL, OpenGL Core, or DX11 is supported.
		 *  
		 *  \return Flag that indicates which rendering APIs are supported. */
		// Which Maya rendering back ends are supported by this plug-in?
		MHWRender::DrawAPI supportedDrawAPIs() const override;

		//! Loop through all render operations and return the current active operation to Maya
		/*! Simple implementation of the Maya render operation loop as seen in the SDK samples. For more information,
		 *  please refer to the Autodesk documentation or the SDK samples.
		 *  
		 *  \return Current render operation. */
		MHWRender::MRenderOperation* renderOperation() override;
		
		//! Update function, called whenever the viewport needs to be rendered / refreshed
		/*! This is where the most important code happens, as this is where the Wisp rendering framework render loop is
		 *  implemented.
		 *  
		 *  \return Returns kSuccess upon successful execution. */
		MStatus setup(const MString& destination) override;

		//! A simple check that checks whether all render operations are valid (no nullptr)
		/*! \return True if everything is correct, else, false. */
		bool AreAllRenderOperationsSetCorrectly() const;

		//! Send the Wisp texture data to the Maya textures
		/*! This function is reponsible for updating the existing textures with Wisp renderer data, as well as recreating
		 *  textures when the viewport is resized.
		 *
		 *  \param maya_renderer The Maya renderer Singleton instance.
		 *  \param texture_manager The Maya texture manager instance retrieved from the Maya renderer.
		 *  \param cpu_textures Data structure that holds the Wisp renderer output data.
		 *  
		 *  \return Returns false if an error occurred, else, true is returned. */
		bool UpdateTextures(MHWRender::MRenderer* maya_renderer, MHWRender::MTextureManager* texture_manager, const wr::CPUTextures& cpu_textures);

		//! Copy raw Wisp renderer texture data to a Maya texture
		/*! This function handles the actual copying of texture data from the Wisp render output data structures to the
		 *  Maya textures. Based on the buffer type passed to this function, one of the Maya textures will be updated.
		 *  For instance, if the type is depth, the Maya depth texture will receieve the data. Same thing for the color
		 *  buffer: passing the color type will result in the Wisp render output data being copied to the Maya color texture.
		 *  
		 *  \param texture_to_update Texture that should receive the data.
		 *  \param type The type of buffer data that should be copied.
		 *  \param cpu_texture The Wisp renderer output texture data to copy from.
		 *  \param texture_manager Maya texture manager instance.
		 *  
		 *  \sa WispBufferType */
		void UpdateTextureData(MHWRender::MTextureAssignment& texture_to_update, WispBufferType type, const wr::CPUTexture& cpu_texture, MHWRender::MTextureManager* texture_manager);

		//! Clean-up the viewport override
		/*! Sets the current render operation to -1 and always returns kSuccess.
		 *
		 *  \return kSuccess is always returned from this function, it simply cannot fail. */
		MStatus cleanup() override;

		//! Returns the name of the plug-in that should show up under the "renderer" drop-down menu in the Maya viewport
		/*! \return Name of the drop-down menu item. */
		MString uiName() const override;

		//! Sets the current render operation to 0
		/*! \return Always returns true no matter what. This function cannot fail. */
		bool startOperationIterator() override;

		//! Move to the next render operation
		/*! Advance the current render operation index by one. Maya will reset the render operation index as soon as this
		 *  function returns true (indicating that the final operation has finished executing).
		 *  
		 *  \return True when the last operation has been selected, else, false. */
		bool nextRenderOperation() override;

		//! Mirror the Maya camera in the Wisp rendering framework
		/*! To make Maya usable, the Wisp framework camera has to match the Maya viewport camera exactly. This function
		 *  grabs the Maya camera matrices and converts it to the Wisp format. */
		void SynchronizeWispWithMayaViewportCamera();

	private:
		//! Frame graph manager that correctly configures the Wisp frame graph render passes
		std::unique_ptr<FrameGraphManager> m_frame_graph_manager;

		//! Name of the ui panel that will be overridden
		MString m_ui_name;

		//! All render operations used in this plug-in
		std::array<std::unique_ptr<MHWRender::MRenderOperation>, 4> m_render_operations;

		//! Custom render operation names for the overrides
		MString m_render_operation_names[3];

		//! Index of the currently active render operation
		int m_current_render_operation;

		//! Plug-in color buffer description
		MHWRender::MTextureDescription m_color_texture_desc;

		//! Plug-in color buffer texture
		MHWRender::MTextureAssignment m_color_texture;

		//! Plug-in depth buffer description
		MHWRender::MTextureDescription m_depth_texture_desc;

		//! Plug-in depth buffer texture
		MHWRender::MTextureAssignment m_depth_texture;

		//! Wisp framework model loading using the Assimp model loader
		std::unique_ptr<wr::AssimpModelLoader> m_model_loader;

		//! Viewport camera
		std::shared_ptr<wr::CameraNode> m_viewport_camera;

		//! Wisp framework render system
		std::unique_ptr<wr::D3D12RenderSystem> m_render_system;

		//! Wisp framework scene graph
		std::shared_ptr<wr::SceneGraph> m_scenegraph;

		//! Wisp framework texture pool
		std::shared_ptr<wr::TexturePool> m_texture_pool;

		std::unique_ptr<wmr::ScenegraphParser> m_scenegraph_parser;
	};
}