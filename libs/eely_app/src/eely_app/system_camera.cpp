#include "eely_app/system_camera.h"

#include "eely_app/component_camera.h"
#include "eely_app/component_transform.h"

#include <eely/math/math_utils.h>
#include <eely/math/transform.h>

#include <entt/entity/registry.hpp>

#include <SDL_scancode.h>

namespace eely {
void system_camera_update(app& app, entt::registry& registry, const float dt_s)
{
  const inputs& inputs{app.get_inputs()};

  auto view{registry.view<component_transform, component_camera>()};
  for (entt::entity entity : view) {
    component_transform& component_transform{view.get<eely::component_transform>(entity)};
    component_camera& component_camera{view.get<eely::component_camera>(entity)};

    // Apply inputs, if pinning is on
    if (app.get_mouse_pinning()) {
      // Rotation

      const input_analog& cursor_relative{inputs.get_mouse_cursor_relative()};

      static constexpr float coordinate_to_deg{deg_to_rad(0.1F)};
      component_camera.yaw += cursor_relative.x * coordinate_to_deg;
      component_camera.pitch += cursor_relative.y * coordinate_to_deg;
      component_camera.pitch = std::clamp(component_camera.pitch, -pi / 2.0F, pi / 2.0F);

      // Translation

      const float3 forward{
          vector_rotate(float3{0.0F, 0.0F, 1.0F}, component_transform.transform.rotation)};
      const float3 right{
          vector_rotate(float3{1.0F, 0.0F, 0.0F}, component_transform.transform.rotation)};

      const input_digital forward_button{inputs.get_keyboard_button(SDL_SCANCODE_W)};
      const input_digital back_button{inputs.get_keyboard_button(SDL_SCANCODE_S)};
      const input_digital right_button{inputs.get_keyboard_button(SDL_SCANCODE_D)};
      const input_digital left_button{inputs.get_keyboard_button(SDL_SCANCODE_A)};

      static constexpr float speed{3.0F};

      const float offset{speed * dt_s};

      if (forward_button == input_digital::just_pressed ||
          forward_button == input_digital::pressed) {
        component_transform.transform.translation += forward * offset;
      }

      if (back_button == input_digital::just_pressed || back_button == input_digital::pressed) {
        component_transform.transform.translation -= forward * offset;
      }

      if (right_button == input_digital::just_pressed || right_button == input_digital::pressed) {
        component_transform.transform.translation += right * offset;
      }

      if (left_button == input_digital::just_pressed || left_button == input_digital::pressed) {
        component_transform.transform.translation -= right * offset;
      }
    }

    component_transform.transform.rotation = quaternion_from_yaw_pitch_roll_intrinsic(
        component_camera.yaw, component_camera.pitch, 0.0F);
  }
}
}  // namespace eely