#include "QuadRendererOverride.hpp"

wmr::WispQuadRenderer::WispQuadRenderer(const MString & t_name)
	: MQuadRender(t_name)
	, m_shader_instance(nullptr)
	, m_color_texture_changed(false)
{
	m_color_texture.texture = nullptr;
}

wmr::WispQuadRenderer::~WispQuadRenderer()
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
}

const MHWRender::MShaderInstance* wmr::WispQuadRenderer::shader()
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
	}

	if (status != MStatus::kSuccess)
	{
		return nullptr;
	}

	return m_shader_instance;
}

MHWRender::MClearOperation& wmr::WispQuadRenderer::clearOperation()
{
	mClearOperation.setClearGradient(false);
	mClearOperation.setMask(static_cast<unsigned int>(MHWRender::MClearOperation::kClearAll));
	
	return mClearOperation;
}

void wmr::WispQuadRenderer::SetColorTexture(const MHWRender::MTextureAssignment& t_color_texture)
{
	m_color_texture.texture = t_color_texture.texture;
	m_color_texture_changed = true;
}
