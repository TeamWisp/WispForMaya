cmake_minimum_required(VERSION 3.13)

# The Maya installation directory location is required for this project to work
if (NOT EXISTS "$ENV{MAYA_2018_DIR}")
    message(FATAL_ERROR "\"MAYA_2018_DIR\" environment variable could not be found!")
endif()

# === Output directories === #
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# === Configuration === #
set(PROJECT_NAME "MayaRayTracedViewport")

set(PLUGIN_EXTENSION ".mll")

set(MAYA_INSTALLATION_DIR $ENV{MAYA_2018_DIR})

# Windows uses backslashes in its paths, to keep things working properly, the backslashes are converted into slashes
STRING(REGEX REPLACE "\\\\" "/" MAYA_INSTALLATION_DIR ${MAYA_INSTALLATION_DIR})

set(MAYA_INCLUDE_DIR "${MAYA_INSTALLATION_DIR}/include")
set(MAYA_LIBRARY_DIR "${MAYA_INSTALLATION_DIR}/lib")

message(STATUS "====================================")
message(STATUS "=== Maya environment information ===")
message(STATUS "====================================")
message(STATUS "Maya installation path:\t" ${MAYA_INSTALLATION_DIR})
message(STATUS "Maya include path:\t\t" ${MAYA_INCLUDE_DIR})
message(STATUS "Maya library path:\t\t" ${MAYA_LIBRARY_DIR}\n)

project(${PROJECT_NAME})

# === Files === #
file(GLOB HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# === Maya API === #
include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIBRARY_DIR})

# TODO: make this crossplatform (e.g. .lib or .so as a prefix)
set(LIBRARIES "OpenMaya.lib" "Foundation.lib")

# A Maya plug-in needs to be built as a shared library, it is not a standalone executable
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

# A Maya plug-in needs its own custom (.mll) extension
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${PLUGIN_EXTENSION})

# Change the startup project to this project (not the CMake projects)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})