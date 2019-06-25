// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <maya/MPxCommand.h>

namespace wmr
{
	const constexpr char* PIPELINE_SHORT_FLAG = "-p";
	const constexpr char* SKYBOX_SHORT_FLAG = "-sb";
	const constexpr char* DOF_FILM_SIZE_SHORT_FLAG = "-fs";
	const constexpr char* DOF_BOKEH_SHAPE_SIZE_SHORT_FLAG = "-bss";
	const constexpr char* DOF_APERTURE_BLADE_COUNT_SHORT_FLAG = "-abc";
	const constexpr char* DOF_AUTO_FOCUS_SHORT_FLAG = "-af";
	const constexpr char* BLOOM_ENABLE_SHORT_FLAG = "-be";

	const constexpr char* PIPELINE_LONG_FLAG = "-pipeline";
	const constexpr char* SKYBOX_LONG_FLAG = "-skybox";
	const constexpr char* DOF_FILM_SIZE_LONG_FLAG = "-dof_film_size";
	const constexpr char* DOF_BOKEH_SHAPE_SIZE_LONG_FLAG = "-dof_bokeh_shape_size";
	const constexpr char* DOF_APERTURE_BLADE_COUNT_LONG_FLAG = "-dof_aperature_blade_count";
	const constexpr char* DOF_AUTO_FOCUS_LONG_FLAG = "-dof_auto_focus";
	const constexpr char* BLOOM_ENABLE_LONG_FLAG = "-enable_bloom";

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

	private:
	};
}