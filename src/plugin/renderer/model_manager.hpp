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

// Wisp rendering framework
#include "wisp.hpp"

// Maya API
#include <maya/MString.h>

// C++ standard
#include <memory>
#include <unordered_map>
#include <vector>

namespace wr
{
	class RenderSystem;
}

namespace wmr
{
	class ModelManager
	{
	public:
		ModelManager() = default;
		~ModelManager() = default;

		//! Create all required structures
		void Initialize();

		//! Request to load a model, if the model already exists
		/*! Returns a pointer to the loaded model.
		 *
		 *  /return Pointer to the loaded model. */
		wr::Model* AddModel(const wr::MeshData<wr::Vertex>& data) noexcept;

		//! Update existing mode data
		void UpdateModel( wr::Model& model, const wr::MeshData<wr::Vertex>& data );

		//! Delete Model from pool
		void DeleteModel( wr::Model& model );

		//! Deallocate used resources
		void Destroy() noexcept;

	private:
		std::shared_ptr<wr::ModelPool> m_model_pool;		//! Wisp object for model loading
	};

}
