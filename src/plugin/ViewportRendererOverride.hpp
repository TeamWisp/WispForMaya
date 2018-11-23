#pragma once

#include <maya/MViewport2Renderer.h>

namespace wisp
{
	class ViewportRendererOverride : public MHWRender::MRenderOverride
	{
	public:
		ViewportRendererOverride(const MString& t_name);
		~ViewportRendererOverride() final override;

		// This plug-in supports all rendering APIs (only has to show the output of the Wisp renderer)
		MHWRender::DrawAPI supportedDrawAPIs() const final override;

		bool startOperationIterator() final override;

		MHWRender::MRenderOperation* renderOperation() final override;

		bool nextRenderOperation() final override;

		MString uiName() const final override;

		MStatus setup(const MString& t_destination) final override;
		MStatus cleanup() final override;

	private:
		// This is the name that will appear in the "Renderer" menu drop-down
		MString m_plugin_name;

		int m_render_operation_iterator;
	};
}
