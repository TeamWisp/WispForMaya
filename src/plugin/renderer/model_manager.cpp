#include "model_manager.hpp"

// Wisp plug-in
#include "miscellaneous/functions.hpp"

// Wisp rendering framework
#include "wisp.hpp"

wmr::ModelManager::ModelManager()
	: m_model_pool(std::make_unique<wr::ModelPool>())
{
}

wmr::ModelManager::~ModelManager()
{
}

wr::Model* wmr::ModelManager::AddModel(const MString& name, const wr::MeshData<wr::Vertex>& data, bool& replaced_existing_model) noexcept
{
	// Assume there was no existing model that could be replaced
	replaced_existing_model = false;

	// Convert the MString to a hash to make it faster to look it up
	auto hash = func::HashCString(name.asChar());

	// Load a model using the new model data
	auto model = m_model_pool->LoadCustom<wr::Vertex>({ data });

	// If the model exists already, delete the old model
	if (m_models.find(hash) != m_models.end())
	{
		m_model_pool->Destroy(m_models[hash]);

		// Make the caller aware that a model has been destroyed and replaced again
		replaced_existing_model = true;
	}

	// Save the most recent model
	m_models[hash] = model;

	// Just to avoid yet another call to "GetModelByName", the pointer is returned here already
	return model;
}

wr::Model* wmr::ModelManager::GetModelByName(const char* name) noexcept
{
	auto hash = func::HashCString(name);

	// If the model exists, return it
	if (m_models.find(hash) != m_models.end())
		return m_models[hash];
	
	// Model does not exist
	return nullptr;
}

wr::Model* wmr::ModelManager::GetModelByName(const MString& name) noexcept
{
	auto hash = func::HashCString(name.asChar());

	// If the model exists, return it
	if (m_models.find(hash) != m_models.end())
	{
		return m_models[hash];
	}

	// Model does not exist
	return nullptr;
}
