#pragma once

#include <eely/math/float3.h>
#include <eely/math/float4.h>
#include <eely/math/math_utils.h>
#include <eely/math/transform.h>

#include <gsl/util>

#include <array>

namespace eely {
// Row-major 4x4 matrix.
// Vectors are assumed to be row-vectors, thus order of transformations is read
// and applied from left to right. E.g. `v * M0 * M1` == `v * (M0 * M1)`
// applies `M0` matrix to vector `v` first and `M1` is applied second.
struct matrix4x4 final {
  static const matrix4x4 identity;

  // Create uninitialized matrix.
  matrix4x4();

  // Create matrix with specified values.
  matrix4x4(float m00,
            float m01,
            float m02,
            float m03,

            float m10,
            float m11,
            float m12,
            float m13,

            float m20,
            float m21,
            float m22,
            float m23,

            float m30,
            float m31,
            float m32,
            float m33);

  [[nodiscard]] float operator()(gsl::index r, gsl::index c) const;
  [[nodiscard]] float& operator()(gsl::index r, gsl::index c);

  [[nodiscard]] const float* data() const;

private:
  std::array<std::array<float, 4>, 4> _data;
};

// Describes in what range depth values should be
// after transformation to clip space and division by w.
enum class clip_space_depth_range {
  // Depth is within [0; 1] (e.g. Direct3D, Metal).
  zero_to_plus_one,

  // Depth is within [-1; 1] (e.g. OpenGL).
  minus_one_to_plus_one
};

// Describes parameters required to generate clip space transformation.
struct clip_space_params final {
  // Camera's horizontal field of view.
  float fov_x{pi / 3.0F};

  // Viewport aspect ratio (x/y).
  float aspect_ratio_x_to_y{1.0F};

  // Near plane's z coordinate in camera space.
  float near{0.001F};

  // Far plane's z coordinate in camera space.
  float far{100.0F};

  // Range of depth values, different conventions are used for different APIs.
  clip_space_depth_range depth_range{clip_space_depth_range::minus_one_to_plus_one};
};

// Return result of multiplication of two matrices. */
[[nodiscard]] matrix4x4 operator*(const matrix4x4& m0, const matrix4x4& m1);

// Return `true` if two matrices are exactly the same.
[[nodiscard]] bool operator==(const matrix4x4& m0, const matrix4x4& m1);

// Return `true` if two matrices are not exactly the same.
[[nodiscard]] bool operator!=(const matrix4x4& m0, const matrix4x4& m1);

// Return matrix that represents translation transformation.
[[nodiscard]] matrix4x4 matrix4x4_from_translation(float x, float y, float z);

// Return matrix that represents scaling transformation.
[[nodiscard]] matrix4x4 matrix4x4_from_scale(float x, float y, float z);

// Return matrix that represents rotation transformation.
[[nodiscard]] matrix4x4 matrix4x4_from_rotation(const quaternion& q);

// Return matrix that represents specified transformation.
[[nodiscard]] matrix4x4 matrix4x4_from_transform(const transform& t);

// Return matrix that transforms from camera space to clip space.
// Returned matrix assumes that camera looks in +Z direction in left-handed
// cooridnate space.
[[nodiscard]] matrix4x4 matrix4x4_clip_space(const clip_space_params& params);

// Return transposed matrix.
[[nodiscard]] matrix4x4 matrix4x4_transpose(const matrix4x4& m);

// Return location transformed by specified matrix.
[[nodiscard]] float3 matrix4x4_transform_location(const matrix4x4& m, const float3& l);

// Return direction transformed by specified matrix
[[nodiscard]] float3 matrix4x4_transform_direction(const matrix4x4& m, const float3& d);

// Return homogeneous vector transformed by specified matrix
[[nodiscard]] float4 matrix4x4_transform(const matrix4x4& m, const float4& f);

// Return inverse of a matrix.
[[nodiscard]] auto matrix4x4_inverse(const matrix4x4& m) -> matrix4x4;

// Return `true` if corresponding components of two `matrix4x4`s are within
// specified epsilon.
[[nodiscard]] bool matrix4x4_near(const matrix4x4& m0,
                                  const matrix4x4& m1,
                                  float epsilon = epsilon_default);

// Implementation

inline float matrix4x4::operator()(gsl::index r, gsl::index c) const
{
  return _data.at(r).at(c);
}

inline float& matrix4x4::operator()(gsl::index r, gsl::index c)
{
  return _data.at(r).at(c);
}

inline const float* matrix4x4::data() const
{
  return &_data[0][0];
}
}  // namespace eely