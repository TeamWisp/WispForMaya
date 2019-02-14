#pragma once

// Maya API
#include <maya/MShaderManager.h>

// Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! User-interface override implementation
	/*! Implementation of a Maya MSceneRender. It inherits from the Maya API base class and implements all methods needed
	 *  to make the override work. The code style for functions is a bit different here because our style guide differs
	 *  from the style used for the Maya API. */
	class WispUIRenderer : public MHWRender::MSceneRender
	{
	public:
		//! Sets the name of the base class to the argument value
		WispUIRenderer(const MString& name);

		//! Unused
		virtual ~WispUIRenderer() = default;

	private:
		//! Filter used to render the user-interface
		/*! Please note that this is an implementation of a Maya API function. Please refer to the Autodesk documentation
		 *  for more information.
		 *  
		 *  \return Scene renderer filter type. */
		MHWRender::MSceneRender::MSceneFilterOption renderFilterOverride() final override;

		//! Configure the clear operation for the user-interface renderer
		/*! Please note that this is a function override from the Maya API, so check the Autodesk documentation for more
		 *  information.
		 *
		 *  \return Returns the clear operation data structure as seen in the Maya API. */
		MHWRender::MClearOperation& clearOperation() final override;
		
		//! Configure what the renderer should render
		/*! Please note that this is an implementation of a Maya API function. Please refer to the Autodesk documentation
		 *  for more information.
		 *  
		 *  return Flag value, check the Autodesk documentation for specific values that can be used here. */
		MUint64 getObjectTypeExclusions() final override;
	};
}