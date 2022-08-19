project(example_btree)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/example_btree/app_example_btree.h
    src/example_btree/app_example_btree.cpp
    src/example_btree/main.cpp
  RESOURCE_FILES
    res/jog.fbx
    res/look_-45.fbx
    res/look_45.fbx
    res/run_crouch.fbx
    res/run.fbx
    res/walk_crouch.fbx
    res/walk.fbx)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)