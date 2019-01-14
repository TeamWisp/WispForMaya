#include "CallbackManager.hpp"
#include <maya\MCallbackIdArray.h>

namespace wmr
{
	CallbackManager* CallbackManager::m_instance = nullptr;

	CallbackManager::CallbackManager()
	{
	}

	CallbackManager::~CallbackManager()
	{
		Reset();
	}

	CallbackManager& CallbackManager::GetInstance()
	{
		return m_instance == nullptr ? *( m_instance = new CallbackManager() ) : *m_instance;
	}

	void CallbackManager::RegisterCallback( MCallbackId mcid )
	{
		m_callback_vector.push_back( mcid );
	}

	void CallbackManager::Reset()
	{
		size_t count = m_callback_vector.size();
		if( count > 0 )
		{
			MCallbackIdArray( m_callback_vector.data(), count );
			m_callback_vector.clear();
		}
	}
}

