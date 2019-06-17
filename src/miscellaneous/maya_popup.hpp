#pragma once

// STD includes
#include <string>

//! Generic plug-in namespace (Wisp Maya Renderer)
namespace wmr
{
	//! Maya popup spawner
	/*! This class can spawn popup's in Maya. All functions are static, so there's no use of actually instancing the class. */
	class MayaPopup final
	{
	public:
		struct Options
		{
			bool btn_ok = true; /**< Defines if the popup should have an "Ok" button */
			std::string window_title = std::string("WispForMaya"); /**< The title of the window */
			std::string window_name = std::string("unique_name"); /**< A unique name for the maya window */
			uint32_t width = 400; /**< The width of the popup */
		};

		//! Spawn a popup with some text from a string
		/*!
			\param content The popup content in plain ASCII text.
			\param options [Optional] The options for this popup
		*/
		static void Spawn(std::string& content, const Options& options = Options()) noexcept;

		//! Spawn a popup with some text from a stringstream
		/*!
			\param content The popup content in plain ASCII text.
			\param options [Optional] The options for this popup 
		*/
		static void Spawn(std::stringstream &content, const Options &options = Options()) noexcept;

		//! Spawn a popup with text loaded from a file
		/*!
			\param path The path to the location of the file
			\param options [Optional] The options for this popup
			\return A boolean that defines if the file could be loaded or not. `true` is it was loaded correctly.
		*/
		static bool SpawnFromFile(const char * path, const Options& options = Options()) noexcept;

	};
}