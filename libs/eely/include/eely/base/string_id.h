#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <gsl/util>

#include <bit>
#include <string>

namespace eely {
// TODO: hashed string
using string_id = std::string;

namespace internal {
static constexpr gsl::index string_id_max_size{255};
static constexpr gsl::index bits_string_id_size{
    std::bit_width(static_cast<size_t>(string_id_max_size))};

// Return `string_id` value read from a memory buffer.
template <>
string_id bit_reader_read(bit_reader& reader);

// Write `string_id` into a memory buffer.
void bit_writer_write(bit_writer& writer, const string_id& id);

// Implementation

template <>
inline string_id bit_reader_read(bit_reader& reader)
{
  const auto size{bit_reader_read<gsl::index>(reader, bits_string_id_size)};

  string_id result;
  result.resize(size);

  for (gsl::index i{0}; i < size; ++i) {
    result[i] = bit_reader_read<char>(reader);
  }

  return result;
}
}  // namespace internal
}  // namespace eely