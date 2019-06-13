cmake_minimum_required(VERSION 3.14)

add_compile_definitions(_ENABLE_EXTENDED_ALIGNED_STORAGE)

# The Maya installation directory location is required for this project to work
if (NOT EXISTS "$ENV{MAYA_SDK_DIR}")
    message(FATAL_ERROR "\"MAYA_SDK_DIR\" environment variable could not be found!")
endif()

# === Output directories === #
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# === Configuration === #
set(PROJECT_NAME "WispForMaya")
set(PLUGIN_EXTENSION ".mll")
set(MAYA_INSTALLATION_DIR $ENV{MAYA_SDK_DIR})

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
file(GLOB PLUGIN_RENDERER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/renderer/*.hpp")
file(GLOB PLUGIN_RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/renderer/*.cpp")
file(GLOB PLUGIN_FRAMEGRAPH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/framegraph/*.hpp")
file(GLOB PLUGIN_FRAMEGRAPH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/framegraph/*.cpp")
file(GLOB WISP_RENDER_TASKS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/wisp_render_tasks/*.hpp")
file(GLOB WISP_RENDER_TASKS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/wisp_render_tasks/*.cpp")

# === Source groups === #
source_group("Main" FILES ${HEADERS} ${SOURCES})
source_group("Miscellaneous" FILES ${MISC_HEADERS} ${MISC_SOURCES})
source_group("" FILES ${PLUGIN_HEADERS} ${PLUGIN_SOURCES})
source_group("Render Operations" FILES ${PLUGIN_RENDER_OPERATIONS_HEADERS} ${PLUGIN_RENDER_OPERATIONS_SOURCES})
source_group("Parsers" FILES ${PLUGIN_PARSERS_HEADERS} ${PLUGIN_PARSERS_SOURCES})
source_group("Renderer" FILES ${PLUGIN_RENDERER_HEADERS} ${PLUGIN_RENDERER_SOURCES})
source_group("Framegraph" FILES ${PLUGIN_FRAMEGRAPH_HEADERS} ${PLUGIN_FRAMEGRAPH_SOURCES})
source_group("Wisp Render Tasks" FILES ${WISP_RENDER_TASKS_HEADERS} ${WISP_RENDER_TASKS_SOURCES})

# === CXX FLAGS === #
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")

# === Dependencies === #
set(WISP_LOG_TO_STDOUT OFF)

add_subdirectory(deps/WispRenderer ${CMAKE_BINARY_DIR}/wisprenderer)

# === Maya API === #
include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIBRARY_DIR})

link_directories(${CMAKE_SOURCE_DIR}/deps/crashpad/out/$(ConfigurationName)/obj/)
link_directories(${CMAKE_SOURCE_DIR}/deps/crashpad/out/$(ConfigurationName)/obj/client/)
link_directories(${CMAKE_SOURCE_DIR}/deps/crashpad/out/$(ConfigurationName)/obj/util/)
link_directories(${CMAKE_SOURCE_DIR}/deps/crashpad/out/$(ConfigurationName)/obj/third_party/mini_chromium/mini_chromium/base/)

# === Maya libraries === #
set(LIBRARIES "OpenMaya.lib" "OpenMayaUI.lib" "OpenMayaRender.lib" "Foundation.lib")

# A Maya plug-in needs to be built as a shared library, it is not a standalone executable
add_library(${PROJECT_NAME} SHARED
            ${HEADERS} ${SOURCES}
            ${MISC_HEADERS} ${MISC_SOURCES}
            ${PLUGIN_HEADERS} ${PLUGIN_SOURCES}
            ${PLUGIN_RENDER_OPERATIONS_HEADERS} ${PLUGIN_RENDER_OPERATIONS_SOURCES}
            ${PLUGIN_PARSERS_HEADERS} ${PLUGIN_PARSERS_SOURCES}
            ${PLUGIN_RENDERER_HEADERS} ${PLUGIN_RENDERER_SOURCES}
            ${PLUGIN_FRAMEGRAPH_HEADERS} ${PLUGIN_FRAMEGRAPH_SOURCES}
            ${WISP_RENDER_TASKS_HEADERS} ${WISP_RENDER_TASKS_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC deps/WispRenderer/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/deps/crashpad)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/deps/crashpad/third_party/mini_chromium/mini_chromium)

target_link_libraries(${PROJECT_NAME} WispRenderer)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
target_link_libraries(${PROJECT_NAME} crashpad_client crashpad_util base)

# Additional properties
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${PLUGIN_EXTENSION}) # A Maya plug-in needs its own custom (.mll) extension
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${CMAKE_BINARY_DIR}/bin/\$\(Configuration\)/${PROJECT_NAME}.mll"      # <--this is in-file
        "${CMAKE_BINARY_DIR}/../module/wisp/plug-ins")                 # <--this is out-file path

if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /MP /Gm-)
endif()

# Change the startup project to this project (not the CMake projects)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../")
