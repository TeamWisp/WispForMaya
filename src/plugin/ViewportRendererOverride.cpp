#include "ViewportRendererOverride.hpp"

wisp::ViewportRendererOverride::ViewportRendererOverride(const MString& t_name)
	: MRenderOverride(t_name)
	, m_plugin_name("Realtime viewport ray-tracer")
{
}

wisp::ViewportRendererOverride::~ViewportRendererOverride()
{
}

MHWRender::DrawAPI wisp::ViewportRendererOverride::supportedDrawAPIs() const
{
	return MHWRender::kAllDevices;
}
