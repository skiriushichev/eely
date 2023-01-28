project(example_state_machine)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_state_machine/app_example_state_machine.h
    src/example_state_machine/app_example_state_machine.cpp
    src/example_state_machine/main.cpp
  RESOURCE_FILES
    res/block_idle.fbx
    res/block_start.fbx
    res/cast_0.fbx
    res/cast_1.fbx
    res/idle_flex.fbx
    res/idle.fbx
    res/slash_0.fbx
    res/slash_1.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()