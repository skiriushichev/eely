project(eely_editor)

eely_add_app(
  TARGET ${PROJECT_NAME}
  SOURCE_FILES
    src/eely_editor/app_editor.h
    src/eely_editor/app_editor.cpp
    src/eely_editor/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE eely_importer)