#include "screen_render_operation.hpp"

namespace wmr
{
	ScreenRenderOperation::ScreenRenderOperation(const MString & name)
		: MQuadRender(name)
		, m_shader_instance(nullptr)
		, m_color_texture_changed(false)
		, m_depth_stencil_state(nullptr)
	{
		m_color_texture.texture = nullptr;
		m_depth_texture.texture = nullptr;
	}

	ScreenRenderOperation::~ScreenRenderOperation()
	{
		// Get a hold of the Maya renderer instance
		MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
			return;

		// Release the shader
		if (m_shader_instance)
		{
			const MHWRender::MShaderManager* maya_shader_manager = maya_renderer->getShaderManager();

			if (maya_shader_manager)
			{
				maya_shader_manager->releaseShader(m_shader_instance);
			}

			m_shader_instance = nullptr;
		}

		// Release the depth stencil state
		if (m_depth_stencil_state)
		{
			MHWRender::MStateManager::releaseDepthStencilState(m_depth_stencil_state);
			m_depth_stencil_state = nullptr;
		}
	}

	const MHWRender::MShaderInstance* ScreenRenderOperation::shader()
	{
		if (!m_shader_instance)
		{
			// Get a hold of the Maya renderer
			MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();
			
			// Get a hold of the Maya shader manager
			const MHWRender::MShaderManager* maya_shader_manager = maya_renderer ? maya_renderer->getShaderManager() : nullptr;

			if (maya_shader_manager)
			{
				m_shader_instance = maya_shader_manager->getEffectsFileShader("wispBlitColorDepth", "");
			}
		}

		// Assume the operation failed
		MStatus status = MStatus::kFailure;

		if (m_shader_instance)
		{
			// If the texture has changed, bind a new texture to the shader
			status = MStatus::kSuccess;

			if (m_color_texture_changed)
			{
				// Use this color texture to render the scene
				status = m_shader_instance->setParameter("gColorTex", m_color_texture);
				m_color_texture_changed = false;
			}

			if (m_depth_texture_changed)
			{
				// Use this depth texture to render the scene
				status = m_shader_instance->setParameter("gDepthTex", m_depth_texture);
				m_depth_texture_changed = false;
			}
		}

		// If the operation succeeded, return the shader instance, else, nullptr
		if (status != MStatus::kSuccess)
			return nullptr;
		else
			return m_shader_instance;
	}

	const MHWRender::MDepthStencilState* ScreenRenderOperation::depthStencilStateOverride()
	{
		if (!m_depth_stencil_state)
		{
			// A description provides Maya with all the necessary information it needs to know about the depth texture
			MHWRender::MDepthStencilStateDesc ds_state_desc;
			ds_state_desc.depthEnable = true;
			ds_state_desc.depthWriteEnable = true;
			ds_state_desc.depthFunc = MHWRender::MStateManager::kCompareLessEqual;

			m_depth_stencil_state = MHWRender::MStateManager::acquireDepthStencilState(ds_state_desc);
		}

		return m_depth_stencil_state;
	}

	MHWRender::MClearOperation& ScreenRenderOperation::clearOperation()
	{
		// Just use a solid color everywhere, no gradient
		mClearOperation.setClearGradient(false);

		// Clear everything there is
		mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearAll));

		return mClearOperation;
	}

	void ScreenRenderOperation::SetColorTexture(const MHWRender::MTextureAssignment& color_texture)
	{
		m_color_texture.texture = color_texture.texture;
		m_color_texture_changed = true;
	}

	void ScreenRenderOperation::SetDepthTexture(const MHWRender::MTextureAssignment& depth_texture)
	{
		m_depth_texture.texture = depth_texture.texture;
		m_depth_texture_changed = true;
	}
}
