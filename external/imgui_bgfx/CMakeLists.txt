project(external_imgui_bgfx)

set(SOURCE_FILES
  "include/imgui_impl_bgfx.h"
  "src/fs_ocornut_imgui.bin.h"
  "src/imgui_impl_bgfx.cpp"
  "src/vs_ocornut_imgui.bin.h")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE external_bgfx external_imgui)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC include)