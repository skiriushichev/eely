#include "eely/base/bit_writer.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace eely::internal {
bit_writer::bit_writer(const std::span<std::byte>& data)
    : _data{data.data()}, _data_size_bits{std::ssize(data) * 8}
{
  EXPECTS(_data != nullptr);
  EXPECTS(_data_size_bits > 0);
}

void bit_writer::write(const write_params& params)
{
  EXPECTS(params.size_bits >= 0 && params.size_bits <= 32);
  EXPECTS(std::bit_width(params.value) <= params.size_bits);

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
  EXPECTS(params.size_bits >= 0 && params.size_bits <= 32);
  EXPECTS(params.offset_bits >= 0 && params.offset_bits <= _data_size_bits);

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

void bit_writer_write(bit_writer& writer, const float value)
{
  static_assert(sizeof(float) == 4);
  writer.write({.value = bit_cast<uint32_t>(value), .size_bits = 32});
}

void bit_writer_write(bit_writer& writer, const bool value)
{
  writer.write({.value = value ? 1U : 0U, .size_bits = 1});
}
}  // namespace eely::internal