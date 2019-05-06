How to install
1.  Unzip archive at preferred location
2. copy wisp-template.mod to:
    1. if MAYA_MODULE_PATH is not set, any of the following directories
        -   <user’s directory>/My Documents/maya/2015/modules

        -   <user’s directory>/My Documents/maya/modules

        -   C:/Program Files/Common Files/Autodesk Shared/Modules/maya/2015

        -   C:/Program Files/Common Files/Autodesk Shared/Modules/maya

        -   <maya_directory>/modules/
    2. if MAYA_MODULE_PATH is set
        -   the specified path
    2. rename wisp-template.mod to wisp.mod
    3. open wisp.mod and edit line 1: <Extracted location> to be equal to the install/extracted location.
3. now to can start Maya and load it as any other plug-in

DO NOT AUTO LOAD PLUG-IN could require reset of preferences to fix.

Known issues
- hard coded skybox.
- crash when using big models. size unknown. memory limited.
- model merging leaves ghost model. (model duplicate in memory?)
- duplication does not work. instant crash.
- using extrude tools work but require forced synchronization.(force a model update by pressing 1/2/3/1, switching viewport 2.0 rendering mode)
- outline not rendered properly in some cases.
- plug-in crashes maya if plug-in gets unloaded
- hidden window not so hidden for debugging purpose. 
- meshes might show black of wrong material. add one more unique material and mesh.

