// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
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

#pragma once

// Maya API
#include <maya/MStatus.h>

// C++ standard
#include <cstdint>

//! Makes it easy to specify buffer sizes
constexpr int operator""MB(unsigned long long int megabytes)
{
	return megabytes * 1024 * 1024;
}

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Holds stand-alone utility functions
	namespace func
	{
		//! Throws an exception upon failure
		/*! This function should only be used on critical systems, as it will trigger a Maya crash. Please do not use this as
		 *  a standard "generic" check.
		 * 
		 *  /param status The status to be evaluated.
		 *	/param msg Message to display when the status equals MStatus::kFailure
		 *  /return No return value, but an exception will be thrown when status == MStatus::kSuccess. */
		void ThrowIfFailedMaya(const MStatus& status, const char* msg);

		//! Output a debug message to the console
		/*! Utility function that logs to the Visual Studio console window on Windows. On other systems, "printf" is used instead.
		 *  If desired, a platform-specific implementation may be created here.
		 *
		 *  /param msg The message to log to the output. */
		void LogDebug(const char* msg);

		//! Hash a c-string
		/*! Implementation from: http://www.cse.yorku.ca/~oz/hash.html .
		 * 
		 *  \param str The string to hash. */
		size_t HashCString(const char* str);

		// https://stackoverflow.com/a/3407254
		//! Round the input number to the nearest multiple of the specified number
		/*! \param input Number to round.
		 *  \param multiple Multiple to round the input to. */
		std::uint32_t RoundUpToNearestMultiple(std::uint32_t input, std::uint32_t multiple);
	}
}
