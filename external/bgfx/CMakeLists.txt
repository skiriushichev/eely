project(external_bgfx)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_definitions(${PROJECT_NAME} INTERFACE BX_CONFIG_DEBUG=0)

add_library(external_bgfx_bx STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_bgfx_bx PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libbx.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_bgfx_bimg STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_bgfx_bimg PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libbimg.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_bgfx_bgfx SHARED IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_bgfx_bgfx PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libbgfx.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(
    ${PROJECT_NAME} INTERFACE
    "-framework AppKit"
    "-framework CoreFoundation"
    "-framework Foundation"
    "-framework IOKit"
    "-framework Metal"
    "-framework QuartzCore")
else()
  message(FATAL_ERROR "System is not supported")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE external_bgfx_bx external_bgfx_bimg external_bgfx_bgfx)