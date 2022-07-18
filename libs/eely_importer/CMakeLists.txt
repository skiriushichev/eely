project(eely_importer)

set(SOURCE_FILES
    include/eely_importer/importer.h
    src/eely_importer/importer.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC eely external_fbxsdk)