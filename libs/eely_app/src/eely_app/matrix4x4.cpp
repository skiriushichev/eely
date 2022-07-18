#include "eely_app/matrix4x4.h"

#include <eely/float3.h>
#include <eely/float4.h>
#include <eely/math_utils.h>
#include <eely/quaternion.h>
#include <eely/transform.h>

#include <gsl/util>

#include <array>
#include <cmath>

namespace eely {
// clang-format off
const matrix4x4 matrix4x4::identity{
    1.0F, 0.0F, 0.0F, 0.0F,
    0.0F, 1.0F, 0.0F, 0.0F,
    0.0F, 0.0F, 1.0F, 0.0F,
    0.0F, 0.0F, 0.0F, 1.0F};
// clang-format on

matrix4x4::matrix4x4() = default;

matrix4x4::matrix4x4(float m00,
                     float m01,
                     float m02,
                     float m03,  // NOLINT(bugprone-easily-swappable-parameters),
                                 // this is a 16 floats ctor wdyw

                     float m10,
                     float m11,
                     float m12,
                     float m13,  // NOLINT(bugprone-easily-swappable-parameters),
                                 // this is a 16 floats ctor wdyw

                     float m20,
                     float m21,
                     float m22,
                     float m23,  // NOLINT(bugprone-easily-swappable-parameters),
                                 // this is a 16 floats ctor wdyw

                     float m30,
                     float m31,
                     float m32,
                     float m33)
{
  _data[0][0] = m00;
  _data[0][1] = m01;
  _data[0][2] = m02;
  _data[0][3] = m03;

  _data[1][0] = m10;
  _data[1][1] = m11;
  _data[1][2] = m12;
  _data[1][3] = m13;

  _data[2][0] = m20;
  _data[2][1] = m21;
  _data[2][2] = m22;
  _data[2][3] = m23;

  _data[3][0] = m30;
  _data[3][1] = m31;
  _data[3][2] = m32;
  _data[3][3] = m33;
}

matrix4x4 operator*(const matrix4x4& m0, const matrix4x4& m1)
{
  matrix4x4 result;

  for (gsl::index r{0}; r < 4; ++r) {
    for (gsl::index c{0}; c < 4; ++c) {
      const float d0{m0(r, 0) * m1(0, c)};
      const float d1{m0(r, 1) * m1(1, c)};
      const float d2{m0(r, 2) * m1(2, c)};
      const float d3{m0(r, 3) * m1(3, c)};

      result(r, c) = d0 + d1 + d2 + d3;
    }
  }

  return result;
}

bool operator==(const matrix4x4& m0, const matrix4x4& m1)
{
  for (gsl::index r{0}; r < 4; ++r) {
    for (gsl::index c{0}; c < 4; ++c) {
      if (m0(r, c) != m1(r, c)) {
        return false;
      }
    }
  }

  return true;
}

bool operator!=(const matrix4x4& m0, const matrix4x4& m1)
{
  return !(m0 == m1);
}

matrix4x4 matrix4x4_from_translation(const float x, const float y, const float z)
{
  // clang-format off
    return matrix4x4{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        x,    y,    z,    1.0F};
  // clang-format on
}

matrix4x4 matrix4x4_from_scale(const float x, const float y, const float z)
{
  // clang-format off
    return matrix4x4{
        x,    0.0F, 0.0F, 0.0F,
        0.0F, y,    0.0F, 0.0F,
        0.0F, 0.0F, z,    0.0F,
        0.0F, 0.0F, 0.0F, 1.0F};
  // clang-format on
}

matrix4x4 matrix4x4_from_rotation(const quaternion& q)
{
  return matrix4x4{
      1.0F - 2.0F * q.y * q.y - 2.0F * q.z * q.z,
      2.0F * q.x * q.y + 2.0F * q.w * q.z,
      2.0F * q.x * q.z - 2.0F * q.w * q.y,
      0.0F,

      2.0F * q.x * q.y - 2.0F * q.w * q.z,
      1.0F - 2.0F * q.x * q.x - 2.0F * q.z * q.z,
      2.0F * q.y * q.z + 2.0F * q.w * q.x,
      0.0F,

      2.0F * q.x * q.z + 2.0F * q.w * q.y,
      2.0F * q.y * q.z - 2.0F * q.w * q.x,
      1.0F - 2.0F * q.x * q.x - 2.0F * q.y * q.y,
      0.0F,

      0.0F,
      0.0F,
      0.0F,
      1.0F,
  };
}

matrix4x4 matrix4x4_from_transform(const transform& t)
{
  return matrix4x4_from_scale(t.scale.x, t.scale.y, t.scale.z) *
         matrix4x4_from_rotation(t.rotation) *
         matrix4x4_from_translation(t.translation.x, t.translation.y, t.translation.z);
}

matrix4x4 matrix4x4_clip_space(const clip_space_params& params)
{
  const float projection_z{1.0F / std::tan(params.fov_x / 2.0F)};

  const float zoom_x{projection_z};
  const float zoom_y{projection_z * params.aspect_ratio_x_to_y};

  const float za{params.depth_range == clip_space_depth_range::zero_to_plus_one
                     ? -params.far / (params.near - params.far)
                     : (params.far + params.near) / (params.far - params.near)};
  const float zb{params.depth_range == clip_space_depth_range::minus_one_to_plus_one
                     ? (2.0F * params.near * params.far) / (params.near - params.far)
                     : params.near * params.far / (params.near - params.far)};

  // clang-format off
    return matrix4x4{
        zoom_x, 0.0F,   0.0F, 0.0F,
        0.0F,   zoom_y, 0.0F, 0.0F,
        0.0F,   0.0F,   za,   1.0F,
        0.0F,   0.0F,   zb,   0.0F};
  // clang-format on
}

matrix4x4 matrix4x4_transpose(const matrix4x4& m)
{
  // clang-format off
  return matrix4x4{
    m(0, 0), m(1, 0), m(2, 0), m(3, 0),
    m(0, 1), m(1, 1), m(2, 1), m(3, 1),
    m(0, 2), m(1, 2), m(2, 2), m(3, 2),
    m(0, 3), m(1, 3), m(2, 3), m(3, 3)};
  // clang-format on
}

float3 matrix4x4_transform_location(const matrix4x4& m, const float3& l)
{
  return float3{l.x * m(0, 0) + l.y * m(1, 0) + l.z * m(2, 0) + m(3, 0),
                l.x * m(0, 1) + l.y * m(1, 1) + l.z * m(2, 1) + m(3, 1),
                l.x * m(0, 2) + l.y * m(1, 2) + l.z * m(2, 2) + m(3, 2)};
}

float3 matrix4x4_transform_direction(const matrix4x4& m, const float3& d)
{
  return float3{d.x * m(0, 0) + d.y * m(1, 0) + d.z * m(2, 0),
                d.x * m(0, 1) + d.y * m(1, 1) + d.z * m(2, 1),
                d.x * m(0, 2) + d.y * m(1, 2) + d.z * m(2, 2)};
}

float4 matrix4x4_transform(const matrix4x4& m, const float4& f)
{
  return float4{f.x * m(0, 0) + f.y * m(1, 0) + f.z * m(2, 0) + f.w * m(3, 0),
                f.x * m(0, 1) + f.y * m(1, 1) + f.z * m(2, 1) + f.w * m(3, 1),
                f.x * m(0, 2) + f.y * m(1, 2) + f.z * m(2, 2) + f.w * m(3, 2),
                f.x * m(0, 3) + f.y * m(1, 3) + f.z * m(2, 3) + f.w * m(3, 3)};
}

matrix4x4 matrix4x4_inverse(const matrix4x4& m)
{
  auto cofactor = [&m](gsl::index cofactor_row, gsl::index cofactor_column) -> float {
    std::array<std::array<float, 3>, 3> submatrix;

    gsl::index submatrix_row{0};
    for (gsl::index r{0}; r < 4; ++r) {
      if (r == cofactor_row) {
        continue;
      }

      gsl::index submatrix_column{0};
      for (gsl::index c{0}; c < 4; ++c) {
        if (c == cofactor_column) {
          continue;
        }

        submatrix.at(submatrix_row).at(submatrix_column) = m(r, c);

        ++submatrix_column;
      }

      ++submatrix_row;
    }

    const float sign{(cofactor_row + cofactor_column) % 2 == 0 ? 1.0F : -1.0F};

    return sign * (submatrix[0][0] *
                       (submatrix[1][1] * submatrix[2][2] - submatrix[1][2] * submatrix[2][1]) +
                   submatrix[0][1] *
                       (submatrix[1][2] * submatrix[2][0] - submatrix[1][0] * submatrix[2][2]) +
                   submatrix[0][2] *
                       (submatrix[1][0] * submatrix[2][1] - submatrix[1][1] * submatrix[2][0]));
  };

  const float cofactor00{cofactor(0, 0)};
  const float cofactor01{cofactor(0, 1)};
  const float cofactor02{cofactor(0, 2)};
  const float cofactor03{cofactor(0, 3)};

  const float determinant{m(0, 0) * cofactor00 + m(0, 1) * cofactor01 + m(0, 2) * cofactor02 +
                          m(0, 3) * cofactor03};

  // clang-format off
    matrix4x4 inversed{
        cofactor00, cofactor(1, 0), cofactor(2, 0), cofactor(3, 0),
        cofactor01, cofactor(1, 1), cofactor(2, 1), cofactor(3, 1),
        cofactor02, cofactor(1, 2), cofactor(2, 2), cofactor(3, 2),
        cofactor03, cofactor(1, 3), cofactor(2, 3), cofactor(3, 3),
    };
  // clang-format on

  for (gsl::index r{0}; r < 4; ++r) {
    for (gsl::index c{0}; c < 4; ++c) {
      inversed(r, c) /= determinant;
    }
  }

  return inversed;
}

bool matrix4x4_near(const matrix4x4& m0, const matrix4x4& m1, float epsilon)
{
  for (gsl::index r{0}; r < 4; ++r) {
    for (gsl::index c{0}; c < 4; ++c) {
      if (!float_near(m0(r, c), m1(r, c), epsilon)) {
        return false;
      }
    }
  }

  return true;
}
}  // namespace eely