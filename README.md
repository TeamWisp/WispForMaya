# Maya-Ray-Traced-Viewport-Plugin
Bringing real-time ray-tracing to the Maya viewport using the Wisp real-time ray-tracing library.

# Installation

## Instructions

1. Copy the .mll file.

   1. Developers, build the project and navigate to ```bin/Debug``` or ```bin/Release```, depending on the chosen configuration. Copy the `.mll` file.
   2. Regular users, grab a copy of the `.mll` file from the releases on GitHub.

2. Paste the `.mll` file in the Autodesk Maya plug-ins folder. Depending on your personal Autodesk Maya configuration, this could be different for you.

   By default, the Autodesk Maya installation folder contains a `/plug-ins` folder, the `.mll` file can be placed in here. However, this is considered bad practice. Instead, create a folder somewhere on your computer and add an **environment variable** that points to the newly created folder. Inside this folder, create three sub-directories: *plug-ins*, *icons*, and *scripts*.

   In both cases, the `.mll` should be pasted in the `/plug-ins` folder.

3. Open the plug-in manager and load the plug-in. Alternatively, the MEL `loadPlugin` command can be used. If these two options fail, restart Autodesk Maya. The plug-in should show up now.

![Plug-in loaded and read to use](./readme_media/maya_plugin_loaded.png)

## Developers

Before running the installer, install the Maya dev-kit (if needed), and create an environment variable called `MAYA_2018_DIR`. This environment variable should point to your `<installation/path>/Autodesk/Maya2018` folder.

![MAYA_2018_DIR environment variable](./readme_media/environment_variable.png)

Download this repository and run the **install.bat** file on your machine. If you are compiling this on a different platform, you may have to run CMake manually.