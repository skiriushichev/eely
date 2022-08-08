#pragma once

#include "eely/math/quaternion.h"

#include <gsl/util>

#include <cstdint>
#include <span>

namespace eely::internal {
// Quantization

struct float_quantize_params final {
  float value{0.0F};
  gsl::index bits_count{0};
  float range_from{0.0F};
  float range_length{0.0F};
};

struct float_dequantize_params final {
  uint16_t data{0};
  gsl::index bits_count{0};
  float range_from{0.0F};
  float range_length{0.0F};
};

// Quantize float in a range into request number of bits (up to 16).
uint16_t float_quantize(const float_quantize_params& params);

// Dequantize float in a range from requested number of bits (up to 16).
float float_dequantize(const float_dequantize_params& params);

// Quantize quaternion into 64 bits, 16 for each component.
std::array<uint16_t, 4> quaternion_quantize(const quaternion& q);

// Dequantize quaternion from 64 bits (so `data` should have at least four `uint16_t`).
quaternion quaternion_dequantize(std::span<const uint16_t> data);

// Implementation

inline float float_dequantize(const float_dequantize_params& params)
{
  EXPECTS(params.bits_count > 0 && params.bits_count <= 16);

  if (float_near(params.range_length, 0.0F)) {
    return params.range_from;
  }

  // TODO: cam max index be put in params?
  const uint16_t max_index{gsl::narrow_cast<uint16_t>((1 << params.bits_count) - 1)};
  EXPECTS(params.data <= max_index);

  const float normalized_value{static_cast<float>(params.data) / static_cast<float>(max_index)};
  const float result{params.range_from + normalized_value * params.range_length};

  return result;
}

inline quaternion quaternion_dequantize(const std::span<const uint16_t> data)
{
  float_dequantize_params params{.bits_count = 16, .range_from = -1.0F, .range_length = 2.0F};

  quaternion result;

  params.data = data[0];
  result.x = float_dequantize(params);

  params.data = data[1];
  result.y = float_dequantize(params);

  params.data = data[2];
  result.z = float_dequantize(params);

  params.data = data[3];
  result.w = float_dequantize(params);

  return result;
}
}  // namespace eely::internal