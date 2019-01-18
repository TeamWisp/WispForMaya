#include "QuadRendererOverride.hpp"

namespace wmr
{
	WispScreenBlitter::WispScreenBlitter(const MString & name)
		: MQuadRender(name)
		, m_shader_instance(nullptr)
		, m_color_texture_changed(false)
		, m_depth_stencil_state(nullptr)
	{
		m_color_texture.texture = nullptr;
		m_depth_texture.texture = nullptr;
	}

	WispScreenBlitter::~WispScreenBlitter()
	{
		MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();

		if (!maya_renderer)
		{
			return;
		}

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

	const MHWRender::MShaderInstance* WispScreenBlitter::shader()
	{
		if (!m_shader_instance)
		{
			MHWRender::MRenderer* maya_renderer = MHWRender::MRenderer::theRenderer();
			const MHWRender::MShaderManager* maya_shader_manager = maya_renderer ? maya_renderer->getShaderManager() : nullptr;

			if (maya_shader_manager)
			{
				// TODO: Write a custom shader that does not use the depth buffer
				m_shader_instance = maya_shader_manager->getEffectsFileShader("mayaBlitColorDepth", "");
			}
		}

		MStatus status = MStatus::kFailure;

		if (m_shader_instance)
		{
			// If the texture has changed, bind a new texture to the shader
			status = MStatus::kSuccess;

			if (m_color_texture_changed)
			{
				status = m_shader_instance->setParameter("gColorTex", m_color_texture);
				m_color_texture_changed = false;
			}

			if (m_depth_texture_changed)
			{
				status = m_shader_instance->setParameter("gDepthTex", m_depth_texture);
				m_depth_texture_changed = false;
			}
		}

		if (status != MStatus::kSuccess)
		{
			return nullptr;
		}

		return m_shader_instance;
	}

	const MHWRender::MDepthStencilState * WispScreenBlitter::depthStencilStateOverride()
	{
		if (!m_depth_stencil_state)
		{
			MHWRender::MDepthStencilStateDesc ds_state_desc;
			ds_state_desc.depthEnable = true;
			ds_state_desc.depthWriteEnable = true;
			ds_state_desc.depthFunc = MHWRender::MStateManager::kCompareLessEqual;

			m_depth_stencil_state = MHWRender::MStateManager::acquireDepthStencilState(ds_state_desc);
		}

		return m_depth_stencil_state;
	}

	MHWRender::MClearOperation& WispScreenBlitter::clearOperation()
	{
		mClearOperation.setClearGradient(false);
		mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearAll));

		return mClearOperation;
	}

	void WispScreenBlitter::SetColorTexture(const MHWRender::MTextureAssignment& color_texture)
	{
		m_color_texture.texture = color_texture.texture;
		m_color_texture_changed = true;
	}

	void WispScreenBlitter::SetDepthTexture(const MHWRender::MTextureAssignment& depth_texture)
	{
		m_depth_texture.texture = depth_texture.texture;
		m_depth_texture_changed = true;
	}
}
