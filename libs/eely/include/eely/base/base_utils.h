#pragma once

#include "eely/base/assert.h"

#include <gsl/pointers>

#include <cstring>
#include <memory>
#include <type_traits>

namespace eely {
struct align_size_to_params final {
  size_t alignment{0};
  size_t size{0};
};

inline size_t align_size_to(const align_size_to_params& params)
{
  size_t aligned_size = params.size;

  const size_t remainder{params.size % params.alignment};
  if (remainder != 0) {
    aligned_size += params.alignment - remainder;
  }

  return aligned_size;
}

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
