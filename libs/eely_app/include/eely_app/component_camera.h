#pragma once

#include <eely/math_utils.h>

namespace eely {
struct component_camera final {
  float fov_x{pi / 3.0F};
  float near{0.001F};
  float far{100.0F};

  float yaw{0.0F};
  float pitch{0.0F};
};
}  // namespace eely