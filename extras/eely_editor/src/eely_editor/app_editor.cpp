#include "eely_editor/app_editor.h"

#include <eely_app/app.h>
#include <eely_app/component_camera.h>
#include <eely_app/component_transform.h>
#include <eely_app/scene.h>
#include <eely_app/system_camera.h>
#include <eely_app/system_render.h>
#include <eely_app/system_skeleton.h>

#include <imgui.h>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0xDCDCDCFF};

app_editor::app_editor(const unsigned int width,
                       const unsigned int height,
                       const std::string& title)
    : app(width, height, title), _scene(*this)
{
  _scene.add_system(&system_skeleton_update);
  _scene.add_system(&system_camera_update);
  _scene.add_system(&system_render_update);

  entt::registry& registry{_scene.get_registry()};

  // Create camera

  entt::entity camera{registry.create()};
  registry.emplace<component_transform>(camera, transform{float3{0.0F, 0.75F, 3.0F}});
  component_camera& component_camera{registry.emplace<eely::component_camera>(camera)};
  component_camera.yaw = pi;
}

void app_editor::update(const float /*dt_s*/)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);
}
}  // namespace eely