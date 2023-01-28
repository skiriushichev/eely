#pragma once

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace eely::internal {
// Allows to write bits into a memory buffer.
class bit_writer final {
public:
  // Data required to write bits into a buffer.
  struct write_params final {
    // Bits to write.
    uint32_t value{0};

    // How many bits from `value` should be written,
    // and for how much a writer should advance.
    gsl::index size_bits{0};
  };

  // Data required to patch bits in a buffer.
  struct patch_params final {
    // Value to write.
    uint32_t value{0};

    // How many bits from `value` should be written.
    gsl::index size_bits{0};

    // At what bit position in a buffer should patching be performed.
    gsl::index offset_bits{0};
  };

  // Construct bit writer targeted at specified buffer.
  explicit bit_writer(const std::span<std::byte>& data);

  // Write specified number of bits from the value into the buffer,
  // starting from current position.
  // Bit position will advance for `params.size_bits`.
  void write(const write_params& params);

  // Write specified number of bits from the value into the buffer,
  // starting from specified offset in bits.
  // Bit position of the writter will not change.
  void patch(const patch_params& params);

  // Move bit position to the start of the next byte.
  // All skipped bits will be set to zero.
  void align();

  // Return current position in bits.
  [[nodiscard]] gsl::index get_bit_position() const;

private:
  std::byte* _data = nullptr;
  gsl::index _data_size_bits;
  gsl::index _position_bits{0};
};

// Return number of bytes written so far.
[[nodiscard]] gsl::index bit_writer_get_bytes_written(const bit_writer& writer);

// Utility functions for writing predefined types.
// Since `bit_writer` only provides base interface (write N bits from an unsinged integer),
// its usage can be verbose (casts, common operations are duplicated etc.).
// These functions aim to reduce that and also provide extensible way to write commonly used types
// and wrappers around them (such as `std::optional<T>`).

// Write `float` value into a memory buffer.
void bit_writer_write(bit_writer& writer, float value);

// Write `bool` into a memory buffer.
void bit_writer_write(bit_writer& writer, bool value);

// Write integral value into a memory buffer with specified bit size.
template <std::integral T>
void bit_writer_write(bit_writer& writer, T value, gsl::index size_bits = sizeof(T) * 8);

// Write scoped enum value into a memory buffer with specified bit size.
template <scoped_enum T>
void bit_writer_write(bit_writer& writer, T value, gsl::index bits_size);

// Write optional value into a memory buffer.
// Underneath value is written as `bit_writer_write(writer, value, args...)`.
template <typename T, typename... A>
void bit_writer_write(bit_writer& writer, const std::optional<T>& opt_value, const A... args);

// Implementation

template <std::integral T>
void bit_writer_write(bit_writer& writer, const T value, const gsl::index size_bits)
{
  EXPECTS(gsl::narrow<gsl::index>(sizeof(T) * 8) >= size_bits);
  writer.write({.value = gsl::narrow<uint32_t>(value), .size_bits = size_bits});
}

template <scoped_enum T>
void bit_writer_write(bit_writer& writer, const T value, const gsl::index bits_size)
{
  bit_writer_write(writer, static_cast<std::underlying_type_t<T>>(value), bits_size);
}

template <typename T, typename... A>
void bit_writer_write(bit_writer& writer, const std::optional<T>& opt_value, const A... args)
{
  if (opt_value.has_value()) {
    bit_writer_write(writer, true);
    bit_writer_write(writer, opt_value.value(), args...);
  }
  else {
    bit_writer_write(writer, false);
  }
}
}  // namespace eely::internal