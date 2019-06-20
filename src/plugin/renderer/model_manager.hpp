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
