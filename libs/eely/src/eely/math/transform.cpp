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

bool transform_near(const transform& t0, const transform& t1, float epsilon)
{
  return float3_near(t0.translation, t1.translation, epsilon) &&
         quaternion_near(t0.rotation, t1.rotation, epsilon) &&
         float3_near(t0.scale, t1.scale, epsilon);
}

void transform_serialize(const transform& t, bit_writer& writer)
{
  float3_serialize(t.translation, writer);
  quaternion_serialize(t.rotation, writer);
  float3_serialize(t.scale, writer);
}

transform transform_deserialize(bit_reader& reader)
{
  return transform{.translation = float3_deserialize(reader),
                   .rotation = quaternion_deserialize(reader),
                   .scale = float3_deserialize(reader)};
}
}  // namespace eely