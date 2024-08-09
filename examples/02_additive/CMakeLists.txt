project(example_additive)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_additive/app_example_additive.h
    src/example_additive/app_example_additive.cpp
    src/example_additive/main.cpp
  RESOURCE_FILES
    res/jog.fbx
    res/look_-45.fbx
    res/look_45.fbx
    res/run.fbx
    res/walk.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)