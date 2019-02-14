#include "functions.hpp"

// Windows
#include <Windows.h>

namespace wmr::func
{
	void ThrowIfFailedMaya(const MStatus& status)
	{
		if (status != MStatus::kSuccess)
		{
			throw std::exception();
		}
	}

	void LogDebug(const char* msg)
	{
#if defined(WIN32) && defined(_DEBUG)
		// Log to the Visual Studio debug output
		OutputDebugStringA(msg);
#else
		printf("%s", msg);
#endif
	}
}
