#include "eely/math/quantization.h"

#include <gsl/narrow>

#include <cstdint>

namespace eely::internal {
uint16_t float_quantize(const float_quantize_params& params)
{
  [[maybe_unused]] static constexpr float epsilon_asserts{1e-3F};

  EXPECTS(params.value >= params.range_from &&
          params.value <= params.range_from + params.range_length + epsilon_asserts);
  EXPECTS(params.bits_count > 0 && params.bits_count <= 16);

  if (float_near(params.range_length, 0.0F)) {
    return 0;
  }

  const float normalized_value{(params.value - params.range_from) / params.range_length};
  EXPECTS(normalized_value >= 0.0F && normalized_value <= 1.0F + epsilon_asserts);

  const uint16_t max_index{gsl::narrow<uint16_t>((1 << params.bits_count) - 1)};
  const float scaled_value{normalized_value * static_cast<float>(max_index)};

  // TODO: This truncates towards zero. Different rounding can be considered here
  const uint16_t data{static_cast<uint16_t>(scaled_value)};

  return data;
}

std::array<uint16_t, 4> quaternion_quantize(const quaternion& q)
{
  float_quantize_params params{.bits_count = 16, .range_from = -1.0F, .range_length = 2.0F};

  std::array<uint16_t, 4> result;

  params.value = q.x;
  result[0] = float_quantize(params);

  params.value = q.y;
  result[1] = float_quantize(params);

  params.value = q.z;
  result[2] = float_quantize(params);

  params.value = q.w;
  result[3] = float_quantize(params);

  return result;
}
}  // namespace eely::internal