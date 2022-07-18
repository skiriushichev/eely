project(external_googletest)

add_library(${PROJECT_NAME} INTERFACE)

add_library(external_googletest_gtest STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_googletest_gtest PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libgtest.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_googletest_gtest_main STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_googletest_gtest_main PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libgtest_main.a)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_googletest_gmock STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_googletest_gmock PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libgmock.a
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/include)
else()
  message(FATAL_ERROR "System is not supported")
endif()

add_library(external_googletest_gmock_main STATIC IMPORTED)
if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  set_target_properties(
    external_googletest_gmock_main PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/${EELY_TARGET_SYSTEM_MACOS_ARM}/libgmock_main.a)
else()
  message(FATAL_ERROR "System is not supported")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE external_googletest_gtest external_googletest_gtest_main external_googletest_gmock external_googletest_gmock_main)