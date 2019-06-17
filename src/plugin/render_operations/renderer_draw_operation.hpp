// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zijnstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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

#include <maya/MViewport2Renderer.h>

namespace wmr
{
	// Forward declarations
	class Renderer;

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

	private:
		Renderer& m_renderer;
	};
}