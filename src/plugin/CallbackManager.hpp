#pragma once

#include <maya\MMessage.h>
#include <vector>

namespace wmr
{
	class CallbackManager
	{
	public:
		CallbackManager();
		~CallbackManager();

		static CallbackManager& GetInstance();

		void RegisterCallback( MCallbackId );
		void Reset();

	private:
		static CallbackManager* m_instance;
		std::vector<MCallbackId> m_callback_vector;

	};
}
