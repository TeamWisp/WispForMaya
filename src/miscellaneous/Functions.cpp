#include "Functions.hpp"

namespace wmr
{
	void functions::ThrowIfFailedMaya(const MStatus & status)
	{
		if (status != MStatus::kSuccess)
		{
			throw std::exception();
		}
	}
}
