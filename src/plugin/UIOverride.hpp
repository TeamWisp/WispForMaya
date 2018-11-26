#pragma once
#include <maya/MShaderManager.h>

namespace wmr
{
	class WispUIRenderer : public MHWRender::MSceneRender
	{
	public:
		WispUIRenderer(const MString& t_name);
		~WispUIRenderer() override;

	private:
		MHWRender::MSceneRender::MSceneFilterOption renderFilterOverride() override;
		MHWRender::MClearOperation& clearOperation() override;
		
		MUint64 getObjectTypeExclusions() override;
	};
}