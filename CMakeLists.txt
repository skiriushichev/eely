cmake_minimum_required(VERSION 3.23)

project(
    eely
    VERSION 0.1
    DESCRIPTION "Animation library"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EELY_TARGET_SYSTEM_MACOS_ARM "macos_arm")
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(EELY_TARGET_SYSTEM ${EELY_TARGET_SYSTEM_MACOS_ARM})
    else()
        message(FATAL_ERROR "System is not supported")
    endif()
else()
    message(FATAL_ERROR "System is not supported")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_subdirectory(external/acl)
add_subdirectory(external/bgfx)
add_subdirectory(external/entt)
add_subdirectory(external/fbxsdk)
add_subdirectory(external/fmt)
add_subdirectory(external/googletest)
add_subdirectory(external/gsl)
add_subdirectory(external/imgui)
add_subdirectory(external/imgui_bgfx)
add_subdirectory(external/imgui_node_editor)
add_subdirectory(external/sdl)
add_subdirectory(libs/eely)
add_subdirectory(libs/eely_app)
add_subdirectory(libs/eely_importer)
add_subdirectory(examples/00_clip)
add_subdirectory(examples/01_blend)
add_subdirectory(examples/02_additive)
add_subdirectory(examples/03_state_machine_simple)
add_subdirectory(examples/04_state_machine_complex)
add_subdirectory(examples/05_ik)
add_subdirectory(extras/eely_editor)

add_subdirectory(tests)