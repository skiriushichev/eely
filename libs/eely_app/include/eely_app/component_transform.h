#pragma once

#include <eely/transform.h>

namespace eely {
struct component_transform final {
  transform transform{transform::identity};
};
}  // namespace eely