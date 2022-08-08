#pragma once

#include <gsl/util>

#include <cstddef>
#include <cstdint>
#include <span>

namespace eely {
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
}  // namespace eely