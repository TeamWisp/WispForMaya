#include "CallbackManager.hpp"

// Maya API
#include <maya/MCallbackIdArray.h>

namespace wmr
{
	CallbackManager* CallbackManager::m_instance = nullptr;

	CallbackManager::~CallbackManager()
	{
		// Calling this just in case the user did not...
		Reset();
	}

	CallbackManager& CallbackManager::GetInstance()
	{
		// Return the instance of the class, or nullptr if it has not been set yet
		return m_instance == nullptr ? *(m_instance = new CallbackManager()) : *m_instance;
	}

	void CallbackManager::RegisterCallback(MCallbackId mcid)
	{
		// Save the callback ID for future use
		m_callback_vector.push_back(mcid);
	}

	void CallbackManager::Reset()
	{
		size_t count = m_callback_vector.size();

		// Only reset the callback if there are any in the first place
		if( count > 0 )
		{
			MCallbackIdArray( m_callback_vector.data(), count );
			m_callback_vector.clear();
		}
	}
}

