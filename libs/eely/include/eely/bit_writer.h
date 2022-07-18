#pragma once

#include <gsl/util>

#include <cstddef>
#include <cstdint>

namespace eely {
// Allows to write bits into a memory buffer.
class bit_writer final {
public:
  // Data required to write bits into a buffer.
  struct write_params final {
    // Value to write.
    uint32_t value{0};

    // How many bits from `value` should be written.
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
}  // namespace eely