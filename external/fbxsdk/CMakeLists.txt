project(external_fbxsdk)

find_package(FbxSdk REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Iconv REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)

add_library(external_fbxsdk_base STATIC IMPORTED)
set_target_properties(
  external_fbxsdk_base
  PROPERTIES
    IMPORTED_LOCATION ${FBXSDK_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${FBXSDK_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} INTERFACE external_fbxsdk_base ZLIB::ZLIB ${LIBXML2_LIBRARIES} Iconv::Iconv)