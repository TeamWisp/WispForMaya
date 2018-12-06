#pragma once

#include <maya/MStatus.h>

namespace wmr::functions
{
	void ThrowIfFailedMaya(const MStatus& status);
}
