#pragma once

#include <maya/MPxCommand.h>

namespace wmr
{
	class Renderer;

	class RenderPipelineSelectCommand final : public MPxCommand
	{
	public:
		RenderPipelineSelectCommand() = default;
		virtual ~RenderPipelineSelectCommand() = default;

		// Virtual functions that *must* be overridden
		MStatus doIt(const MArgList& args) override;
		MStatus redoIt() override { return MStatus::kSuccess; }
		MStatus undoIt() override { return MStatus::kSuccess; }
		bool isUndoable() const override { return false; };
		static void* creator() { return new RenderPipelineSelectCommand(); }
		static MSyntax create_syntax();
	};
}