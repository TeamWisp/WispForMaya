// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zijnstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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

#include "functions.hpp"

// Wisp
#include <util/log.hpp>

// Windows
#include <Windows.h>

namespace wmr::func
{
	void ThrowIfFailedMaya(const MStatus& status, const char* msg)
	{
		if (status != MStatus::kSuccess)
		{
			LOGE(msg);
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
