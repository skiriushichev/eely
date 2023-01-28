project(example_blend)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_blend/app_example_blend.h
    src/example_blend/app_example_blend.cpp
    src/example_blend/main.cpp
  RESOURCE_FILES
    res/jog.fbx
    res/run_crouch.fbx
    res/run.fbx
    res/walk_crouch.fbx
    res/walk.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()