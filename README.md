# Maya-Ray-Traced-Viewport-Plugin
Bringing real-time ray-tracing to the Maya viewport using the [Wisp real-time ray-tracing library](https://github.com/TeamWisp/Procedural-Ray-Tracing).

# Table of contents

1. [Installation instructions](#instructions)
2. [Setting the environment variable](#environment-variable)
3. [Configuring the Remote Windows Debugger](#debugging)
   1. [Project properties](#project-properties)
   2. [Changing the debugger](#select-the-remote-windows-debugger)
   3. [Windows Remote Debugger settings](#windows-remote-debugger-settings)
   4. [Using the Windows Remote Debugger](#select-the-windows-remote-debugger)

# Installation

## Instructions

1. Copy the .mll file.

   1. **Developers**, build the project and navigate to ```bin/Debug``` or ```bin/Release```, depending on the chosen configuration. Copy the `.mll` file.
   2. **Regular users**, grab a copy of the `.mll` file from the releases on GitHub.

2. Paste the `.mll` file in the Autodesk Maya plug-ins folder. This folder is different for each user, as it has to be manually specified in the `Maya.env` configuration file. Please refer to [the documentation](https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2018/ENU/Maya-EnvVar/files/GUID-8EFB1AC1-ED7D-4099-9EEE-624097872C04-htm.html) for more information on how to set-up your `Maya.env` file.

   By default, the Autodesk Maya installation folder contains a `/plug-ins` folder, the `.mll` file can be placed in here. However, this is considered bad practice. Instead, create a folder somewhere on your computer and add an **environment variable** that points to the newly created folder. Inside this folder, create three sub-directories: *plug-ins*, *icons*, and *scripts*.

   In both cases, the `.mll` should be pasted in the `/plug-ins` folder.

3. Open the plug-in manager and load the plug-in. Alternatively, the MEL `loadPlugin` command can be used. If these two options fail, restart Autodesk Maya. The plug-in should show up now.

![Plug-in loaded and read to use](./readme_media/maya_plugin_loaded.png)

# Plug-in development

## Environment variable

Before running the installer, install the [Maya dev-kit](https://www.autodesk.com/developer-network/platform-technologies/maya) (if needed), and create an environment variable called `MAYA_2018_DIR`. This environment variable should point to your `<installation/path>/Autodesk/Maya2018` folder.

![MAYA_2018_DIR environment variable](./readme_media/environment_variable.png)

Download this repository and run the **install.bat** file on your machine. If you are compiling this on a different platform, you may have to run CMake manually.

## Debugging

The instructions below assume that the Visual Studio Debugger is used for plug-in development. If you use any other debugger, the guide below does not apply to you.

Please check your debugger documentation to find out how to attach to a running process.

### Project properties
Right-click on the project and open the properties.

![Project properties](readme_media/project_properties.png)

### Select the Remote Windows Debugger

Go to the **debugging** settings. By default, Visual Studio uses the *Local Windows Debugger*, change this into *Remote Windows Debugger*.

![Selecting the remote debugger](readme_media/selecting_remote_debugger.png)

### Windows Remote Debugger settings

After selecting the *Remote Windows Debugger*, new settings will appear. There are two settings that need to be changed:

1. **Remote command**
   The remote command is the command that run once the *Remote Windows Debugger* is launched.
   Set this to the location of `maya.exe`. The location of the executable depends on your installation settings, but by default it is located here: `C:\Program Files\Autodesk\Maya2018\bin\maya.exe`.

   <u>Please note that the backslashes (`\`) are required. Using forward slashes (`/`) will cause the Windows Remote Debugger to fail to attach to the Maya process.</u>

2. **Attach**
   If you were to launch the *Remote Windows Debugger* right now, a new instance of Maya will be launched. This is not something you want when debugging an application, so change this setting to `Yes`.

![Correct Remote Windows Debugger settings](readme_media/debugger_settings_to_attach.png)

### Select the Windows Remote Debugger

To make it easy to launch the *Windows Remote Debugger*, click the arrow next to *Local Windows Debugger* and select the *Remote Windows Debugger*. Now, every time that button is clicked (shortcut: `F5`), the *Remote Windows Debugger* is used instead of the *Local Windows Debugger*.

![Launching the Remote Windows Debugger](readme_media/select_correct_way_to_run.png)

Thanks a lot, [Nick Cullen](https://nickcullen.net/blog/misc-tutorials/how-to-debug-a-maya-2016-c-plugin-using-visual-studio-2015/), for the step-by-step instructions on how to get the Remote Windows Debugger to work with Autodesk Maya 2018.
