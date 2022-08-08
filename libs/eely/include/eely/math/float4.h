#pragma once

namespace eely {
// Structure that combines four floats.
// Their meaning depends on context: homogeneous vector, coefficients etc.
struct float4 final {
  float x{0.0F};
  float y{0.0F};
  float z{0.0F};
  float w{0.0F};
};
}  // namespace eely