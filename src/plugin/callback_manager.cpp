#include "callback_manager.hpp"

#include <algorithm>

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

	void CallbackManager::Destroy()
	{
		delete m_instance;
	}

	void CallbackManager::RegisterCallback(MCallbackId mcid)
	{
		// Save the callback ID for future use
		m_callback_vector.push_back(mcid);
	}

	void CallbackManager::UnregisterCallback(MCallbackId mcid)
	{
		auto it = std::find_if(m_callback_vector.begin(), m_callback_vector.end(), [&mcid] (const std::vector<MCallbackId>::value_type& vt)
		{
			return vt == mcid;
		});
		if (it != m_callback_vector.end())
		{
			MMessage::removeCallback(mcid);
			m_callback_vector.erase(it);
		}
	}

	void CallbackManager::Reset()
	{
		size_t count = m_callback_vector.size();

		// Only reset the callback if there are any in the first place
		if( count > 0 )
		{
			auto cbarray = MCallbackIdArray( m_callback_vector.data(), count );
			MMessage::removeCallbacks( cbarray );
			m_callback_vector.clear();
		}
	}
}

