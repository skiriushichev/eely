#include "eely/math_utils.h"

#include <cmath>

namespace eely {
bool float_near(const float a, const float b, const float epsilon)
{
  return std::abs(a - b) <= epsilon;
}
}  // namespace eely