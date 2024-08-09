cmake_minimum_required(VERSION 3.23)

project(
    eely
    VERSION 0.1
    DESCRIPTION "Animation library"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EELY_PLATFORM_WIN64 "win64")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(EELY_PLATFORM ${EELY_PLATFORM_WIN64})

    set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG DEBUG)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE RELEASE)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL RELEASE)

    add_compile_definitions(EELY_PLATFORM_WIN64)
    add_compile_options(/W4 /WX)
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /Zc:preprocessor /utf-8")
else()
    message(FATAL_ERROR "Platform is not supported")
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

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