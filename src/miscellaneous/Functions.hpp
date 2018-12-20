#pragma once

#include <maya/MStatus.h>

namespace wmr::func
{
	// Throws exception when Status::kSuccess is false
	void ThrowIfFailedMaya(const MStatus& status);

	// Wraps the Visual Studio debug output function nicely
	void LogDebug(const char* msg);
}
