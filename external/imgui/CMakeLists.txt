project(external_imgui)

set(SOURCE_FILES
  "include/imconfig.h"
  "include/imgui_impl_sdl.h"
  "include/imgui_internal.h"
  "include/imgui.h"
  "include/imstb_rectpack.h"
  "include/imstb_textedit.h"
  "include/imstb_truetype.h"
  "src/imgui_demo.cpp"
  "src/imgui_draw.cpp"
  "src/imgui_impl_sdl.cpp"
  "src/imgui_tables.cpp"
  "src/imgui_widgets.cpp"
  "src/imgui.cpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE external_sdl)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC include)