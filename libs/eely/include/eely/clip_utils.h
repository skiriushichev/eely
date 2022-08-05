#pragma once

#include "eely/assert.h"
#include "eely/quaternion.h"
#include "eely/transform.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace eely {
// Describes how clip is compressed.
enum class compression_scheme {
  // Clip is not compressed, all data is played as is.
  // Data format:
  //  - Array of `uint32_t`
  //  - Key is represented by up to 48 bytes (12 elements)
  //    - flags (first 5 bits) + optional joint index (last 11 bits)
  //    - optional time (1 element)
  //    - optional translation (3 elements)
  //    - optional rotation (4 elements)
  //    - optional scale (3 elements)
  uncompressed,

  // Clip is quantized into fixed number of bits.
  // Data format:
  //  - Array of `uint16_t`
  //  - Key is represented by up to 24 bytes (12 elements)
  //    - flags (first 5 bits) + optional joint index (last 11 bits) (1 element)
  //    - optional time (1 element)
  //    - optional translation (3 elements)
  //    - optional rotation (4 elements)
  //    - optional scale (3 elements)
  compressed_fixed
};

static constexpr gsl::index bits_compression_scheme = 1;

// Flags for a key inside a clip data.
enum compression_key_flags {
  has_joint_index = 1 << 0,
  has_time = 1 << 1,
  has_translation = 1 << 2,
  has_rotation = 1 << 3,
  has_scale = 1 << 4,
};

static constexpr gsl::index bits_compression_key_flags = 5;

// Metadata for all clips.
struct clip_metadata {
  virtual ~clip_metadata() = default;

  // Describes what part of transform
  // are changed in an animation for specific joint.
  struct joint_transform_components final {
    gsl::index joint_index{0};
    int components{0};
  };

  // Clip duration in seconds.
  float duration_s{0.0F};

  // Transform components participating in animation for specific joints.
  std::vector<joint_transform_components> joint_components;
};

// Metadata for clip with `compressed_fixed` scheme
struct clip_metadata_compressed_fixed final : public clip_metadata {
  // Describes intervals joint's translation and scales are within.
  // Used for quantization.
  struct joint_range final {
    gsl::index joint_index{0};
    float range_translation_from{0.0F};
    float range_translation_length{0.0F};
    float range_scale_from{0.0F};
    float range_scale_length{0.0F};
  };

  std::vector<joint_range> joint_ranges;
};

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
}  // namespace eely