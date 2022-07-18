#pragma once

namespace eely {
// Describes how clip is compressed.
enum class compression_scheme {
  // Clip is not compressed, all data is played as is.
  // Mostly for debugging and comparison.
  // Data format:
  //  - Array of `uint32_t`
  //  - Key is represented by up to 48 bytes (12 elements)
  //    - flags (first 5 bits) + optional joint index (last 16 bits)
  //    - optional time (1 element)
  //    - optional translation (3 elements)
  //    - optional rotation (4 elements)
  //    - optional scale (3 elements)
  uncompressed,

  // Clip is quantized into fixed number of bits.
  // Data format:
  //  - Array of `uint16_t`
  //  - Key is represented by up to 22 bytes (11 elements)
  //    - flags (first 5 bits) + optional joint index (last 11 bits)
  //    - optional time (1 element)
  //    - optional translation (3 elements)
  //    - optional rotation (3 elements)
  //    - optional scale (3 elements)
  fixed
};

// Flags for a key inside a clip data.
enum compression_key_flags {
  has_joint_index = 1 << 0,
  has_time = 1 << 1,
  has_translation = 1 << 2,
  has_rotation = 1 << 3,
  has_scale = 1 << 4,
};
}  // namespace eely