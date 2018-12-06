#include "Functions.hpp"

void wmr::functions::ThrowIfFailedMaya(const MStatus & status)
{
	if (status != MStatus::kSuccess)
	{
		throw std::exception();
	}
}
