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
		wr::Model* AddModel(const MString& name, const wr::MeshData<wr::Vertex>& data, bool& replaced_existing_model) noexcept;

		//! Update existing mode data
		void UpdateModel( wr::Model& model, const wr::MeshData<wr::Vertex>& data );

		//! Deallocate used resources
		void Destroy() noexcept;

		//! Get a pointer to a registered model, nullptr if the model does not exist
		/*! Please keep in mind that this is a relatively slow operation due to a "find()" call.
		 *  
		 *  \return Pointer to the requested model or nullptr if the model could not be found. */
		wr::Model* GetModelByName(const char* name) noexcept;
		wr::Model* GetModelByName(const MString& name) noexcept;


	private:
		std::shared_ptr<wr::ModelPool> m_model_pool;		//! Wisp object for model loading
		std::unordered_map<size_t, wr::Model*> m_models;	//! Models added to the model pool referenced by hash
	};

}
