project(example_ik)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_ik/app_example_ik.h
    src/example_ik/app_example_ik.cpp
    src/example_ik/main.cpp
  RESOURCE_FILES
    res/sitting_clap.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()