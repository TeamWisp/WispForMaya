#include "ViewportRendererOverride.hpp"

wisp::ViewportRendererOverride::ViewportRendererOverride(const MString& t_name)
	: MRenderOverride(t_name)
	, m_plugin_name("Realtime viewport ray-tracer")
	, m_render_operation_iterator(0)
{
}

wisp::ViewportRendererOverride::~ViewportRendererOverride()
{
}

MHWRender::DrawAPI wisp::ViewportRendererOverride::supportedDrawAPIs() const
{
	return MHWRender::kAllDevices;
}

bool wisp::ViewportRendererOverride::startOperationIterator()
{
	m_render_operation_iterator = 0;
	return true;
}

MHWRender::MRenderOperation* wisp::ViewportRendererOverride::renderOperation()
{
	switch (m_render_operation_iterator)
	{
	default:
		break;
	}

	return nullptr;
}

bool wisp::ViewportRendererOverride::nextRenderOperation()
{
	++m_render_operation_iterator;
	
	// TODO: Need something like this once the render operations are in place
	//return m_render_operation_iterator < m_render_operations.size() ? true : false;

	return false;
}

MString wisp::ViewportRendererOverride::uiName() const
{
	return m_plugin_name;
}

MStatus wisp::ViewportRendererOverride::setup(const MString& t_destination)
{
	return MStatus::kSuccess;
}

MStatus wisp::ViewportRendererOverride::cleanup()
{
	return MStatus::kSuccess;
}
