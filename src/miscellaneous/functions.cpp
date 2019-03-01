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

	size_t HashCString(const char * str)
	{
		size_t h = 5381;
		int c;
		while ((c = *str++))
			h = ((h << 5) + h) + c;
		return h;
	}

	std::uint32_t RoundUpToNearestMultiple(std::uint32_t input, std::uint32_t multiple)
	{
		if (multiple == 0)
			return input;

		std::uint32_t remainder = input % multiple;
		if (remainder == 0)
			return input;

		return input + multiple - remainder;
	}

}
