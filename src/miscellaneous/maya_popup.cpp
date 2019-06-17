// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "maya_popup.hpp"

// Maya includes
#include <maya/MGlobal.h>
#include <maya/MString.h>

// STD includes
#include <fstream>
#include <sstream>


namespace wmr
{
	void MayaPopup::Spawn(std::string& content, const Options& options) noexcept
	{
		std::stringstream s;
		s << content.c_str();

		MayaPopup::Spawn(s, options);
	}

	void MayaPopup::Spawn(std::stringstream &content, const Options &options) noexcept
	{
		std::string str_text_prefix = "text -ww on -align \"left\" -rs on -w ";
		str_text_prefix += std::to_string(options.width);
		str_text_prefix += " \"";
		const char const* text_prefix = str_text_prefix.c_str();
		constexpr const char const* text_postfix = "\";\n";

		// Create window
		MString notify_command("window -title \"");
		
		// Window settings
		// Window title
		notify_command += options.window_title.c_str();
		// Other options
		notify_command += "\" -sizeable off -maximizeButton off -minimizeButton off ";
		// Window "behind the scenes" name
		notify_command += options.window_name.c_str();
		notify_command += ";\n";

		// Set layout
		notify_command += "rowColumnLayout -columnOffset 1 \"both\" 10 -rowOffset 1 \"both\" 15 -nc 1 -cal 1 \"left\";\n";

		// Print text
		std::string line;
		while (std::getline(content, line))
		{
			// Add a space if an empty line was found
			if (line.length() <= 0)
			{
				line += " ";
			}

			// Text settings
			notify_command += text_prefix;
			// Add text
			notify_command += line.c_str();
			// Add end quote
			notify_command += text_postfix;
		}

		// Add empty line for proper spacing
		notify_command += text_prefix;
		notify_command += " ";
		notify_command += text_postfix;

		// Add button to close
		if (options.btn_ok)
		{
			notify_command += "button -enable on -command \"deleteUI ";
			notify_command += options.window_name.c_str();
			notify_command += "\" \"Ok\";\n";

			// Add spacing below the button
			notify_command += text_prefix;
			notify_command += " ";
			notify_command += text_postfix;
		}

		// Display window
		notify_command += "showWindow ";
		notify_command += options.window_name.c_str(); 
		notify_command += ";";

		MGlobal::displayInfo(notify_command);

		// Execute display window command
		MGlobal::executeCommand(notify_command);
	}

	bool MayaPopup::SpawnFromFile(const char* path, const Options& options) noexcept
	{
		// Get file
		std::ifstream infile(path);
		if (infile.is_open())
		{
			std::stringstream s;
			s << infile.rdbuf();
			infile.close();

			MayaPopup::Spawn(s, options);
			return true;
		}
		return false;
	}
}

