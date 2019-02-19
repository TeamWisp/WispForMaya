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
file(GLOB MISC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/miscellaneous/*.hpp")
file(GLOB MISC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/miscellaneous/*.cpp")
file(GLOB PLUGIN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.hpp")
file(GLOB PLUGIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.cpp")
file(GLOB PLUGIN_RENDER_OPERATIONS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/render_operations/*.hpp")
file(GLOB PLUGIN_RENDER_OPERATIONS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/render_operations/*.cpp")
file(GLOB PLUGIN_PARSERS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/parsers/*.hpp")
file(GLOB PLUGIN_PARSERS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/parsers/*.cpp")
# === Source groups === #
source_group("Main" FILES ${HEADERS} ${SOURCES})
source_group("Miscellaneous" FILES ${MISC_HEADERS} ${MISC_SOURCES})
source_group("Plugin" FILES ${PLUGIN_HEADERS} ${PLUGIN_SOURCES})
source_group("Plugin Render Operations" FILES ${PLUGIN_RENDER_OPERATIONS_HEADERS} ${PLUGIN_RENDER_OPERATIONS_SOURCES})
source_group("Plugin Parsers" FILES ${PLUGIN_PARSERS_HEADERS} ${PLUGIN_PARSERS_SOURCES})

# === CXX FLAGS === #
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

# === Dependencies === $
add_subdirectory(deps/Procedural-Ray-Tracing ${CMAKE_BINARY_DIR}/proceduralraytracer)

# === Maya API === #
include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIBRARY_DIR})

# === Maya libraries === #
set(LIBRARIES "OpenMaya.lib" "OpenMayaUI.lib" "OpenMayaRender.lib" "Foundation.lib")

# A Maya plug-in needs to be built as a shared library, it is not a standalone executable
add_library(${PROJECT_NAME} SHARED
            ${HEADERS} ${SOURCES}
            ${MISC_HEADERS} ${MISC_SOURCES}
            ${PLUGIN_HEADERS} ${PLUGIN_SOURCES}
            ${PLUGIN_RENDER_OPERATIONS_HEADERS} ${PLUGIN_RENDER_OPERATIONS_SOURCES}
            ${PLUGIN_PARSERS_HEADERS} ${PLUGIN_PARSERS_SOURCES})


target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC deps/Procedural-Ray-Tracing/src)

target_link_libraries(${PROJECT_NAME} WispRenderer)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Additional properties
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${PLUGIN_EXTENSION}) # A Maya plug-in needs its own custom (.mll) extension
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /MP /Gm-)
endif()

if(ENABLE_UNIT_TEST)
    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    #add_subdirectory(deps/googletest/googletest)
    file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/test/*.hpp")
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")

    set_target_properties (gtest PROPERTIES FOLDER ThirdParty)
    set_target_properties (gtest_main PROPERTIES FOLDER ThirdParty)

    if (MSVC)
        target_compile_options(gtest PRIVATE /W0)
    endif()

    add_executable(MayaUnitTest WIN32
                    ${HEADERS} ${SOURCES}
                    ${MISC_HEADERS} ${MISC_SOURCES}
                    ${PLUGIN_HEADERS} ${PLUGIN_SOURCES}
                    ${PLUGIN_RENDER_OPERATIONS_HEADERS} ${PLUGIN_RENDER_OPERATIONS_SOURCES}
                    ${TEST_HEADERS} ${TEST_SOURCES} 
                    ${PLUGIN_PARSERS_HEADERS} ${PLUGIN_PARSERS_SOURCES})

    set_target_properties(MayaUnitTest PROPERTIES CXX_STANDARD 17)
    set_target_properties(MayaUnitTest PROPERTIES CXX_EXTENSIONS OFF)
    set_target_properties(MayaUnitTest PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)

    target_include_directories(MayaUnitTest PUBLIC src)
    target_include_directories(MayaUnitTest PUBLIC deps/Procedural-Ray-Tracing/src)
    target_include_directories(MayaUnitTest PUBLIC deps/googletest/googletest/include)
    target_include_directories(MayaUnitTest PUBLIC ${MAYA_INCLUDE_DIR})
    link_directories(MayaUnitTest PUBLIC ${MAYA_LIBRARY_DIR})

    target_link_libraries(MayaUnitTest gtest)
    target_link_libraries(MayaUnitTest WispRenderer)
    target_link_libraries(MayaUnitTest ${LIBRARIES})
    if (MSVC)
	    target_compile_options(MayaUnitTest PRIVATE /W4 /permissive- /MP /Gm-)
    endif()

endif(ENABLE_UNIT_TEST)

# Change the startup project to this project (not the CMake projects)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../")