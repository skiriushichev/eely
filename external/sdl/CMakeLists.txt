project(external_sdl)

add_library(${PROJECT_NAME} INTERFACE)

add_library(external_sdl_base SHARED IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_sdl_base PROPERTIES
    IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libSDL2.a
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libSDL2-2.0.dylib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_sdl_main STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_sdl_main PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libSDL2main.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE external_sdl_base external_sdl_main)