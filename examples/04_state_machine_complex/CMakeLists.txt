project(example_state_machine_complex)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_state_machine_complex/app_example_state_machine_complex.h
    src/example_state_machine_complex/app_example_state_machine_complex.cpp
    src/example_state_machine_complex/main.cpp
  RESOURCE_FILES
    res/block_idle.fbx
    res/block_start.fbx
    res/cast_0.fbx
    res/cast_1.fbx
    res/idle.fbx
    res/slash_0.fbx
    res/slash_1.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()