#pragma once
#include <maya/MShaderManager.h>

namespace wisp
{
	class UIOverride : public MHWRender::MSceneRender
	{
	public:
		UIOverride(const MString& t_name);
		~UIOverride();

	private:

		MHWRender::MSceneRender::MSceneFilterOption renderFilterOverride() final override;
		MUint64 getObjectTypeExclusions() final override;
		MHWRender::MClearOperation& clearOperation() final override;
	};
}