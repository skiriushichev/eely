if (${EELY_TARGET_SYSTEM} STREQUAL ${EELY_TARGET_SYSTEM_MACOS_ARM})
  file(GLOB_RECURSE FBXSDK_INCLUDE_DIRS "/Applications/Autodesk/FBX SDK/*/fbxsdk.h")
  if (FBXSDK_INCLUDE_DIRS)
    list(GET FBXSDK_INCLUDE_DIRS 0 FBXSDK_INCLUDE_DIR)
  endif()

  get_filename_component(FBXSDK_INCLUDE_DIR ${FBXSDK_INCLUDE_DIR} DIRECTORY)
  get_filename_component(FBXSDK_ROOT_DIR ${FBXSDK_INCLUDE_DIR} DIRECTORY)

  find_library(
    FBXSDK_LIBRARY
    "libfbxsdk.a"
    PATHS ${FBXSDK_ROOT_DIR}
    PATH_SUFFIXES "lib/clang/release")

  if (FBXSDK_INCLUDE_DIR AND FBXSDK_LIBRARY)
    set(FBXSDK_FOUND YES)
  else()
    set(FBXSDK_FOUND NO)
  endif()
else()
  set(FBXSDK_FOUND NO)
  message(FATAL_ERROR "System is not supported")
endif()