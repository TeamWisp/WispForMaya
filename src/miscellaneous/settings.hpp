#pragma once

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Holds all global settings for the plug-in
	namespace settings
	{
		//! Name of the studio / company developing this product
		static const constexpr char* COMPANY_NAME = "Team Wisp";

		//! Name of the product itself
		static const constexpr char* PRODUCT_NAME = "Realtime ray-traced viewport by Team Wisp";
		
		//! Current release version of the product
		static const constexpr char* PRODUCT_VERSION = "0.0.1";
		
		//! Name of the viewport panel this plug-in will override
		static const constexpr char* VIEWPORT_PANEL_NAME = "modelPanel4";

		static const constexpr char* VIEWPORT_OVERRIDE_NAME = "wisp_viewport_override";
	};
}
