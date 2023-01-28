#include "eely/base/bit_reader.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace eely::internal {
bit_reader::bit_reader(const std::span<const std::byte>& data)
    : _data{data.data()}, _data_size_bits{std::ssize(data) * 8}
{
  EXPECTS(_data != nullptr);
  EXPECTS(_data_size_bits > 0);
}

uint32_t bit_reader::read(const gsl::index size_bits)
{
  EXPECTS(size_bits >= 0 && size_bits <= 32);

  if (size_bits == 0) {
    return 0;
  }

  if (_position_bits + size_bits > _data_size_bits) {
    throw std::runtime_error("Attempt to read past specified buffer");
  }

  // Starting byte of bit string
  const gsl::index byte_index = _position_bits / 8;

  // Starting bit index inside starting byte
  const gsl::index bit_index = _position_bits % 8;

  // Number of bits left in starting byte
  const gsl::index remaining_bits = 8 - bit_index;

  // NOLINTNEXTLINE (intentionally using pointer arithmetics)
  uint32_t result = std::to_integer<uint32_t>(_data[byte_index]) >> bit_index;
  if (size_bits > remaining_bits) {
    uint32_t v = 0;

    if (size_bits - remaining_bits > 24) {
      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      v |= std::to_integer<uint32_t>(_data[byte_index + 4]) << 24;
    }
    if (size_bits - remaining_bits > 16) {
      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      v |= std::to_integer<uint32_t>(_data[byte_index + 3]) << 16;
    }
    if (size_bits - remaining_bits > 8) {
      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      v |= std::to_integer<uint32_t>(_data[byte_index + 2]) << 8;
    }

    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    v |= std::to_integer<uint32_t>(_data[byte_index + 1]);

    v <<= remaining_bits;
    result |= v;
  }

  if (size_bits < 32) {
    result &= (1U << size_bits) - 1;
  }

  _position_bits += size_bits;

  return result;
}

gsl::index bit_reader::get_position_bits() const
{
  return _position_bits;
}

gsl::index bit_reader_get_bytes_read(const bit_reader& reader)
{
  return (reader.get_position_bits() + 7) / 8;
}
}  // namespace eely::internal