#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/math/float3.h"
#include "eely/math/math_utils.h"
#include "eely/math/quaternion.h"

#include <gsl/util>

#include <optional>

namespace eely {
// Represents coordinate system relative to some other (parent) space.
// Essentially combines scale, rotation and translation transformations (in this exact order).
struct transform final {
  // TODO: when using inverse transforms with non-uniform scales and rotations,
  // they should be applied via special function, because it needs to be TRS instead of SRT.

  static const transform identity;

  float3 translation{float3::zeroes};
  quaternion rotation{quaternion::identity};
  float3 scale{float3::ones};
};

// Parts of transform.
enum transform_components { translation = 1 << 0, rotation = 1 << 1, scale = 1 << 2 };

// Return transform that is a combination of two other transforms,
// `t0` is applied first and then `t1`.
// Essentialy converts `t1` from `t0` space to `t0`s parent space.
transform operator*(const transform& t0, const transform& t1);

// Return `true` if two transforms are exactly the same.
bool operator==(const transform& t0, const transform& t1);

// Return `true` if two transforms are not exactly the same.
bool operator!=(const transform& t0, const transform& t1);

// Convert location from space represented by `t0` to its parent space.
float3 transform_location(const transform& t0, const float3& l);

// Calculate inverse transform.
transform transform_inverse(const transform& t);

// Calculate difference between two transforms.
// That is, first transform multiplied by difference equals to second transform.
transform transform_diff(const transform& t0, const transform& t1);

// Return `true` if corresponding components of two transforms
// are within specified epsilon.
bool transform_near(const transform& t0, const transform& t1, float epsilon = epsilon_default);

// Serialize `transform` into specified memory buffer.
void transform_serialize(const transform& t, bit_writer& writer);

// Deserialize `transform` from specified memory buffer.
transform transform_deserialize(bit_reader& reader);

namespace internal {
// Number of bits in which it is safe to pack `transform_components` value.
static constexpr gsl::index bits_transform_components{3};

// Template helper structure to deduce component type from enum.
template <transform_components>
struct transform_component_t final {
};

template <>
struct transform_component_t<transform_components::translation> final {
  using type = float3;
  using optional_type = std::optional<float3>;
};

template <>
struct transform_component_t<transform_components::rotation> final {
  using type = quaternion;
  using optional_type = std::optional<quaternion>;
};

template <>
struct transform_component_t<transform_components::scale> final {
  using type = float3;
  using optional_type = std::optional<float3>;
};
}  // namespace internal
}  // namespace eely