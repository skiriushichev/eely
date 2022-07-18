#include "eely/bit_writer.h"

#include <gsl/assert>
#include <gsl/narrow>
#include <gsl/util>

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace eely {
bit_writer::bit_writer(const std::span<std::byte>& data)
    : _data{data.data()}, _data_size_bits{gsl::narrow<gsl::index>(data.size() * 8)}
{
  Expects(_data != nullptr);
  Expects(_data_size_bits > 0);
}

void bit_writer::write(const write_params& params)
{
  Expects(params.size_bits >= 0 && params.size_bits <= 32);

  if (params.size_bits == 0) {
    return;
  }

  if (_position_bits + params.size_bits > _data_size_bits) {
    throw std::runtime_error("Attempt to write past specified buffer");
  }

  patch({.value = params.value, .size_bits = params.size_bits, .offset_bits = _position_bits});

  _position_bits += params.size_bits;
}

void bit_writer::patch(const patch_params& params)
{
  Expects(params.size_bits >= 0 && params.size_bits <= 32);
  Expects(params.offset_bits >= 0 && params.offset_bits <= _data_size_bits);

  if (params.offset_bits + params.size_bits > _data_size_bits) {
    throw std::runtime_error("Attempt to write past specified buffer");
  }

  // Starting byte where to write bit string
  const gsl::index byte_index = params.offset_bits / 8;

  // Starting bit index inside starting byte
  const gsl::index bit_index = params.offset_bits % 8;

  // Number of bits left in starting byte
  const gsl::index remaining_bits = 8 - bit_index;

  if (params.size_bits == 1) {
    const auto mask = static_cast<std::byte>(1 << bit_index);
    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] &= ~mask;
    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] |= static_cast<std::byte>(params.value & 0x01) << bit_index;
  }
  // Value bits do not cross byte boundary
  else if (params.size_bits <= remaining_bits) {
    const auto mask1 = static_cast<std::byte>((1 << bit_index) - 1);
    const auto mask2 = static_cast<std::byte>(0xFF << (bit_index + params.size_bits));

    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] &= mask1 | mask2;
    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] |=
        static_cast<std::byte>((params.value & std::to_integer<uint32_t>(~mask2)) << bit_index);
  }
  else {
    const auto mask = static_cast<std::byte>((1 << bit_index) - 1);

    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] &= mask;
    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] |= static_cast<std::byte>(params.value << bit_index);

    uint32_t byte_offset = 1;
    uint32_t value_shift = remaining_bits;
    const uint32_t full_bytes = (params.size_bits - remaining_bits) / 8;
    const uint32_t tail_bits = (params.size_bits - remaining_bits) % 8;
    for (uint32_t i = 0; i < full_bytes; ++i) {
      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      _data[byte_index + byte_offset] = static_cast<std::byte>(params.value >> value_shift);
      value_shift += 8;
      byte_offset += 1;
    }

    if (tail_bits != 0) {
      const auto tail_mask = static_cast<std::byte>((1 << tail_bits) - 1);

      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      _data[byte_index + byte_offset] &= ~tail_mask;
      // NOLINTNEXTLINE (intentionally using pointer arithmetics)
      _data[byte_index + byte_offset] |=
          static_cast<std::byte>(params.value >> value_shift) & tail_mask;
    }
  }
}

void bit_writer::align()
{
  if ((_position_bits % 8) != 0) {
    const gsl::index byte_index = _position_bits / 8;
    const gsl::index bit_index = _position_bits % 8;

    // NOLINTNEXTLINE (intentionally using pointer arithmetics)
    _data[byte_index] &= static_cast<std::byte>((1 << bit_index) - 1);
    _position_bits += 8 - bit_index;
  }
}

gsl::index bit_writer::get_bit_position() const
{
  return _position_bits;
}

gsl::index bit_writer_get_bytes_written(const bit_writer& writer)
{
  return (writer.get_bit_position() + 7) / 8;
}
}  // namespace eely