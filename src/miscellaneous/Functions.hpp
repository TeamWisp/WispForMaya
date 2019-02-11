#pragma once

// Maya API
#include <maya/MStatus.h>

namespace wmr::func
{
	//! Throws an exception upon failure
	/*! This function should only be used on critical systems, as it will trigger a Maya crash. Please do not use this as
	 *! a standard "generic" check.
	 *!
	 *! \param status The status to be evaluated.
	 *! \return No return value, but an exception will be thrown when status == MStatus::kSuccess. */
	void ThrowIfFailedMaya(const MStatus& status);

	//! Output a debug message to the console
	/*! Utility function that logs to the Visual Studio console window on Windows. On other systems, "printf" is used instead.
	 *! If desired, a platform-specific implementation may be created here.
	 *!
	 *! \param msg The message to log to the output. */
	void LogDebug(const char* msg);
}
