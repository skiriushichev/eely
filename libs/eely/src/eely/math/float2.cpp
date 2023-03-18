#include "eely/math/float2.h"

namespace eely {
bool operator==(const float2& a, const float2& b)
{
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const float2& a, const float2& b)
{
  return !(a == b);
}

bool float2_near(const float2& a, const float2& b, const float epsilon)
{
  return float_near(a.x, b.x, epsilon) && float_near(a.y, b.y, epsilon);
}
}  // namespace eely