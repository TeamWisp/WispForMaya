#pragma once

#include <maya/MStatus.h>

namespace wmr::functions
{
	// Throws exception when Status::kSuccess is false
	void ThrowIfFailedMaya(const MStatus& status);
}
