#pragma once
#include <maya/MShaderManager.h>

namespace wmr
{
	class WispUIRenderer : public MHWRender::MSceneRender
	{
	public:
		WispUIRenderer(const MString& t_name);
		~WispUIRenderer() final override;

	private:
		MHWRender::MSceneRender::MSceneFilterOption renderFilterOverride() final override;
		MHWRender::MClearOperation& clearOperation() final override;
		
		MUint64 getObjectTypeExclusions() final override;
	};
}