#include "eely/math/transform.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/math/float3.h"
#include "eely/math/math_utils.h"
#include "eely/math/quaternion.h"

namespace eely {
const transform transform::identity{float3{0.0F, 0.0F, 0.0F}, quaternion{0.0F, 0.0F, 0.0F, 1.0F},
                                    float3{1.0F, 1.0F, 1.0F}};

transform operator*(const transform& t0, const transform& t1)
{
  const float3 translation{transform_location(t0, t1.translation)};
  const quaternion rotation{t0.rotation * t1.rotation};
  const float3 scale{t0.scale * t1.scale};

  return transform{translation, rotation, scale};
}

bool operator==(const transform& t0, const transform& t1)
{
  return t0.translation == t1.translation && t0.rotation == t1.rotation && t0.scale == t1.scale;
}

bool operator!=(const transform& t0, const transform& t1)
{
  return !(t0 == t1);
}

float3 transform_location(const transform& t0, const float3& l)
{
  float3 result{l * t0.scale};
  result = vector_rotate(result, t0.rotation);
  result = result + t0.translation;

  return result;
}

transform transform_inverse(const transform& t)
{
  transform inverse;
  inverse.rotation = quaternion_normalized(quaternion_inverse(t.rotation));
  inverse.scale = float3{1.0F / t.scale.x, 1.0F / t.scale.y, 1.0F / t.scale.z};
  inverse.translation = vector_rotate(-t.translation * inverse.scale, inverse.rotation);

  EXPECTS(transform_near(t * inverse, transform::identity, 1e-3F));

  return inverse;
}

transform transform_diff(const transform& t0, const transform& t1)
{
  return transform_inverse(t1) * t0;
}

bool transform_near(const transform& t0, const transform& t1, float epsilon)
{
  return float3_near(t0.translation, t1.translation, epsilon) &&
         quaternion_near(t0.rotation, t1.rotation, epsilon) &&
         float3_near(t0.scale, t1.scale, epsilon);
}

namespace internal {
void bit_writer_write(bit_writer& writer, const transform& value)
{
  bit_writer_write(writer, value.translation);
  bit_writer_write(writer, value.rotation);
  bit_writer_write(writer, value.scale);
}
}  // namespace internal
}  // namespace eely