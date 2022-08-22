project(example_clip)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_clip/app_example_clip.h
    src/example_clip/app_example_clip.cpp
    src/example_clip/main.cpp
  RESOURCE_FILES
    res/sitting_clap.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()