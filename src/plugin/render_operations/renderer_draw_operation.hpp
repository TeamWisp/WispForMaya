#pragma once

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	class RendererDrawOperation final : public MHWRender::MUserRenderOperation
	{
	public:
		RendererDrawOperation(const MString& name);
		~RendererDrawOperation();

	private:
		const MCameraOverride* cameraOverride() override;

		MStatus execute(const MDrawContext& draw_context) override;

		bool hasUIDrawables() const override;
		bool requiresLightData() const override;
	};
}