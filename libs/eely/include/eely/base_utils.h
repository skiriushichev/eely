#pragma once

#include "eely/assert.h"

#include <cstring>
#include <type_traits>

namespace eely {
// Convert from `TSrc` to `TDst` type.
// Converted object will have the same bit representation,
// as the source object.
template <typename TDst, typename TSrc>
std::enable_if_t<sizeof(TDst) == sizeof(TSrc) && std::is_trivially_copyable_v<TSrc> &&
                     std::is_trivially_copyable_v<TDst>,
                 TDst>
bit_cast(const TSrc& src) noexcept
{
  static_assert(std::is_trivially_constructible_v<TDst>);

  TDst dst;
  std::memcpy(&dst, &src, sizeof(TDst));
  return dst;
}

template <typename T>
bool has_flag(const typename std::underlying_type<T>::type value, const T flag)
{
  return (value & flag) != 0;
}

template <class TDst, class TSrc>
TDst polymorphic_downcast(TSrc src)
{
  EXPECTS(dynamic_cast<TDst>(src) == src);
  return static_cast<TDst>(src);
}
}  // namespace eely