#pragma once

#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

namespace wisp
{
	class ViewportRendererOverride : public MHWRender::MRenderOverride
	{
	public:
		ViewportRendererOverride(const MString& t_name);
		~ViewportRendererOverride();

		// This plug-in supports all rendering APIs (only has to show the output of the Wisp renderer)
		MHWRender::DrawAPI supportedDrawAPIs() const final override;
		
	private:
		// This is the name that will appear in the "Renderer" menu drop-down
		MString m_plugin_name;
	};
}
