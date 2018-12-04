cmake_minimum_required(VERSION 3.13)

add_compile_definitions(_ENABLE_EXTENDED_ALIGNED_STORAGE)

option(ENABLE_UNIT_TEST "enables unit tests." OFF)

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
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB PLUGIN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.hpp")
file(GLOB PLUGIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.cpp")
file(GLOB PLUGIN_OVERRIDE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/overrides/*.hpp")
file(GLOB PLUGIN_OVERRIDE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/overrides/*.cpp")
file(GLOB PLUGIN_RENDERER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/renderer/*.hpp")
file(GLOB PLUGIN_RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/renderer/*.cpp")

# === Source groups === #
source_group("Plugin Start" FILES ${PLUGIN_HEADERS} ${PLUGIN_SOURCES})
source_group("Plugin Overrides" FILES ${PLUGIN_OVERRIDE_HEADERS} ${PLUGIN_OVERRIDE_SOURCES})
source_group("Plugin Renderer" FILES ${PLUGIN_RENDERER_HEADERS} ${PLUGIN_RENDERER_SOURCES})

# === CXX FLAGS === #
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

# === Dependencies === $
add_subdirectory(deps/Procedural-Ray-Tracing)

# === Maya API === #
include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIBRARY_DIR})

# === Maya libraries === #
set(LIBRARIES "OpenMaya.lib" "OpenMayaUI.lib" "OpenMayaRender.lib" "Foundation.lib")

# A Maya plug-in needs to be built as a shared library, it is not a standalone executable
add_library(${PROJECT_NAME} SHARED
            ${HEADERS} ${SOURCES}
            ${PLUGIN_HEADERS} ${PLUGIN_SOURCES}
            ${PLUGIN_OVERRIDE_HEADERS} ${PLUGIN_OVERRIDE_SOURCES}
            ${PLUGIN_RENDERER_HEADERS} ${PLUGIN_RENDERER_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC deps/Procedural-Ray-Tracing/src)

target_link_libraries(${PROJECT_NAME} WispRenderer)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Additional properties
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${PLUGIN_EXTENSION}) # A Maya plug-in needs its own custom (.mll) extension
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /MP /Gm-)
endif()

if(ENABLE_UNIT_TEST)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
    add_subdirectory(deps/googletest/googletest)
    file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.hpp")
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")

    set_target_properties (gtest PROPERTIES FOLDER ThirdParty)
    set_target_properties (gtest_main PROPERTIES FOLDER ThirdParty)

    set_target_properties( gmock PROPERTIES )

    if (MSVC)
        target_compile_options(gtest PRIVATE /W0)
    endif()

    add_executable(UnitTest WIN32
                    ${HEADERS} ${SOURCES}
                    ${PLUGIN_HEADERS} ${PLUGIN_SOURCES}
                    ${PLUGIN_OVERRIDE_HEADERS} ${PLUGIN_OVERRIDE_SOURCES}
                    ${PLUGIN_RENDERER_HEADERS} ${PLUGIN_RENDERER_SOURCES}
                    ${TEST_HEADERS} ${TEST_SOURCES} )

    target_include_directories(UnitTest PUBLIC src)
    target_include_directories(UnitTest PUBLIC deps/Procedural-Ray-Tracing/src)
    target_include_directories(UnitTest PUBLIC deps/googletest/googletest/include)
    
    target_link_libraries(UnitTest gtest)
    target_link_libraries(UnitTest WispRenderer)
    target_link_libraries(UnitTest ${LIBRARIES})

endif(ENABLE_UNIT_TEST)

# Change the startup project to this project (not the CMake projects)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
