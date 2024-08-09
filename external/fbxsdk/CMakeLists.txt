project(external_fbxsdk)

find_package(FbxSdk REQUIRED)
find_package(LibXml2 REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)

add_library(external_fbxsdk_base STATIC IMPORTED)
set_target_properties(
  external_fbxsdk_base
  PROPERTIES
    IMPORTED_LOCATION_DEBUG ${FBXSDK_LIBRARY_DEBUG}
    IMPORTED_LOCATION_RELEASE ${FBXSDK_LIBRARY_RELEASE}
    INTERFACE_INCLUDE_DIRECTORIES ${FBXSDK_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} INTERFACE external_fbxsdk_base ${LIBXML2_LIBRARIES} ${ZLIB_LIBRARY})