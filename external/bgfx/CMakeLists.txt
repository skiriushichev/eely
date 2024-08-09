project(external_bgfx)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_definitions(${PROJECT_NAME} INTERFACE BX_CONFIG_DEBUG=0)

if (${EELY_PLATFORM} STREQUAL ${EELY_PLATFORM_WIN64})
  add_library(external_bgfx_bx STATIC IMPORTED)
  set_target_properties(
    external_bgfx_bx PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/bxDebug.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/bxRelease.lib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
  
  add_library(external_bgfx_bx_compat INTERFACE)
  target_include_directories(external_bgfx_bx_compat INTERFACE ${PROJECT_SOURCE_DIR}/include/bx_compat/msvc)

  add_library(external_bgfx_bimg STATIC IMPORTED)
  set_target_properties(
    external_bgfx_bimg PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/bimgDebug.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/bimgRelease.lib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
  
  add_library(external_bgfx_bgfx STATIC IMPORTED)
  set_target_properties(
    external_bgfx_bgfx PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/bgfxDebug.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/bgfxRelease.lib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "Platform is not supported")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE external_bgfx_bx_compat external_bgfx_bx external_bgfx_bimg external_bgfx_bgfx)