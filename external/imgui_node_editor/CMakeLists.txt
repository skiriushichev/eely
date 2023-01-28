project(external_imgui_node_editor)

set(SOURCE_FILES
  "include/crude_json.h"
  "include/imgui_bezier_math.h"
  "include/imgui_bezier_math.inl"
  "include/imgui_canvas.h"
  "include/imgui_extra_math.h"
  "include/imgui_extra_math.inl"
  "include/imgui_node_editor_internal.h"
  "include/imgui_node_editor_internal.inl"
  "include/imgui_node_editor.h"
  "src/crude_json.cpp"
  "src/imgui_canvas.cpp"
  "src/imgui_node_editor_api.cpp"
  "src/imgui_node_editor.cpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE external_imgui)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC include)