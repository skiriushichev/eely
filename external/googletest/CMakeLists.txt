project(external_googletest)

add_library(${PROJECT_NAME} INTERFACE)

add_library(external_googletest_gtest STATIC IMPORTED)
if (${EELY_PLATFORM} STREQUAL ${EELY_PLATFORM_WIN64})
  set_target_properties(
    external_googletest_gtest PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/gtest.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/gtest.lib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)

  add_library(external_googletest_gtest_main STATIC IMPORTED)
  set_target_properties(
    external_googletest_gtest_main PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/gtest_main.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/gtest_main.lib)

  add_library(external_googletest_gmock STATIC IMPORTED)
  set_target_properties(
    external_googletest_gmock PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/gmock.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/gmock.lib
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
  
  add_library(external_googletest_gmock_main STATIC IMPORTED)
  set_target_properties(
    external_googletest_gmock_main PROPERTIES
    IMPORTED_LOCATION_DEBUG ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/debug/gmock_main.lib
    IMPORTED_LOCATION_RELEASE ${PROJECT_SOURCE_DIR}/libs/${EELY_PLATFORM_WIN64}/release/gmock_main.lib)
else()
  message(FATAL_ERROR "Platform is not supported")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE external_googletest_gtest external_googletest_gtest_main external_googletest_gmock external_googletest_gmock_main)