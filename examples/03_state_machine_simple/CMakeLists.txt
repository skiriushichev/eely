project(example_state_machine_simple)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_state_machine_simple/app_example_state_machine_simple.h
    src/example_state_machine_simple/app_example_state_machine_simple.cpp
    src/example_state_machine_simple/main.cpp
  RESOURCE_FILES
    res/idle.fbx
    res/taunt.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()