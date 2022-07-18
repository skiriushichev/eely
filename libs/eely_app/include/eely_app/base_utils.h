#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>

namespace eely {
template <typename T>
inline size_t hash_combine(const size_t seed, const T& value)
{
  namespace fs = std::filesystem;

  const size_t value_hash = [&value]() {
    if constexpr (std::is_same_v<T, fs::path>) {
      return fs::hash_value(fs::absolute(value));
    }
    else {
      return std::hash<T>{}(value);
    }
  }();

  return seed ^ (value_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
}  // namespace eely