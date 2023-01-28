#pragma once

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace eely::internal {
// Allows to read bits from a memory buffer.
class bit_reader final {
public:
  // Construct bit reader targeted at specified buffer.
  explicit bit_reader(const std::span<const std::byte>& data);

  // Read specified number of bits at current position,
  // and advance position for that number of bits.
  // If reading goes beyond given buffer, exception is thrown.
  uint32_t read(gsl::index size_bits);

  // Return current position in bits.
  [[nodiscard]] gsl::index get_position_bits() const;

private:
  const std::byte* _data;
  gsl::index _data_size_bits;
  gsl::index _position_bits{0};
};

// Return number of bytes read so far.
[[nodiscard]] gsl::index bit_reader_get_bytes_read(const bit_reader& reader);

// Utility functions for reading predefined types.
// Since `bit_reader` only provides base interface (read N bits and put into unsigned integer),
// its usage can be verbose (casts, common operations are duplicated etc.).
// These functions aim to reduce that and also provide extensible way to read commonly used types
// and wrappers around them (such as `std::optional<T>`).

// Only intended for specializations and overloads.
template <typename T>
T bit_reader_read(bit_reader&) = delete;

// Return `float` value read from a memory buffer.
template <>
float bit_reader_read<float>(bit_reader& reader);

// Return `bool` value read from a memory buffer.
template <>
bool bit_reader_read(bit_reader& reader);

// Return integral value read from a memory buffer with full size.
template <non_bool_integral T>
T bit_reader_read(bit_reader& reader);

// Return integral value read from a memory buffer with specified bit size.
// `non_bool_integral` is required here so that `bool` had its own version of `bit_reader_read`.
template <non_bool_integral T>
T bit_reader_read(bit_reader& reader, gsl::index bits_size);

// Return scoped enum value read from a memory buffer with specified bit size.
template <scoped_enum T>
T bit_reader_read(bit_reader& reader, gsl::index bits_size);

// Return optional value read from a memory buffer.
// Underneath value is read as `bit_reader_read<T::value_type>(reader)`.
template <optional T>
T bit_reader_read(bit_reader& reader);

// Return optional value read from a memory buffer.
// Underneath value is read as `bit_reader_read<T::value_type>(reader, args...)`.
template <optional T, typename... A>
T bit_reader_read(bit_reader& reader, const A... args);

// Implementation

template <>
inline float bit_reader_read<float>(bit_reader& reader)
{
  static_assert(sizeof(float) == 4);
  return bit_cast<float>(reader.read(32));
}

template <>
inline bool bit_reader_read(bit_reader& reader)
{
  return (reader.read(1) == 1);
}

template <non_bool_integral T>
T bit_reader_read(bit_reader& reader)
{
  return bit_reader_read<T>(reader, sizeof(T) * 8);
}

template <internal::non_bool_integral T>
T bit_reader_read(bit_reader& reader, const gsl::index bits_size)
{
  EXPECTS(gsl::narrow<gsl::index>(sizeof(T) * 8) >= bits_size);
  return gsl::narrow<T>(reader.read(bits_size));
}

template <scoped_enum T>
T bit_reader_read(bit_reader& reader, gsl::index bits_size)
{
  return static_cast<T>(bit_reader_read<std::underlying_type_t<T>>(reader, bits_size));
}

template <optional T>
T bit_reader_read(bit_reader& reader)
{
  const bool has_value{bit_reader_read<bool>(reader)};
  if (has_value) {
    return bit_reader_read<typename T::value_type>(reader);
  }

  return std::nullopt;
}

template <optional T, typename... A>
T bit_reader_read(bit_reader& reader, const A... args)
{
  const bool has_value{bit_reader_read<bool>(reader)};
  if (has_value) {
    return bit_reader_read<typename T::value_type>(reader, args...);
  }

  return std::nullopt;
}
}  // namespace eely::internal