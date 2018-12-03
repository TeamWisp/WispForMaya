#include "Functions.hpp"

void wmr::functions::ThrowIfFailedMaya(const MStatus & t_status)
{
	if (t_status != MStatus::kSuccess)
	{
		throw std::exception();
	}
}
