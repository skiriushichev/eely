if (${EELY_PLATFORM} STREQUAL ${EELY_PLATFORM_WIN64})
  file(GLOB_RECURSE FBXSDK_INCLUDE_DIRS "C:/Program Files/Autodesk/FBX/FBX SDK/*/fbxsdk.h")
  if (FBXSDK_INCLUDE_DIRS)
    list(GET FBXSDK_INCLUDE_DIRS 0 FBXSDK_INCLUDE_DIR)
  endif()

  get_filename_component(FBXSDK_INCLUDE_DIR ${FBXSDK_INCLUDE_DIR} DIRECTORY)
  get_filename_component(FBXSDK_ROOT_DIR ${FBXSDK_INCLUDE_DIR} DIRECTORY)

  find_library(
    FBXSDK_LIBRARY_DEBUG
    "libfbxsdk-md.lib"
    PATHS ${FBXSDK_ROOT_DIR}
    PATH_SUFFIXES "lib/x64/debug")

  find_library(
    FBXSDK_LIBRARY_RELEASE
    "libfbxsdk-md.lib"
    PATHS ${FBXSDK_ROOT_DIR}
    PATH_SUFFIXES "lib/x64/release")

  if (FBXSDK_INCLUDE_DIR AND FBXSDK_LIBRARY_DEBUG AND FBXSDK_LIBRARY_RELEASE)
    set(FBXSDK_FOUND YES)
    
    set(LIBXML2_INCLUDE_DIR ${FBXSDK_INCLUDE_DIR}/libxml2)
    set(LIBXML2_LIBRARY ${FBXSDK_ROOT_DIR}/lib/x64/release/libxml2-md.lib)
    set(ZLIB_LIBRARY ${FBXSDK_ROOT_DIR}/lib/x64/release/zlib-md.lib)
  else()
    set(FBXSDK_FOUND NO)
  endif()
else()
  set(FBXSDK_FOUND NO)
  message(FATAL_ERROR "System is not supported")
endif()