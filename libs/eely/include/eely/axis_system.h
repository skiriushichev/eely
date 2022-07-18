#pragma once

namespace eely {
// Describes how axes are oriented in a project,
// i.e. which axis is an "up" axis, etc.
enum class axis_system {
  // +Y is an up axis,
  // +X is a right axis,
  // +Z is a forward axis,
  // left handed.
  y_up_x_right_z_forward
};
}  // namespace eely