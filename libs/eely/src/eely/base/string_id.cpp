#include "eely/base/string_id.h"

#include "eely/base/assert.h"

#include <gsl/narrow>

#include <bit>
#include <cstdint>

namespace eely {
static constexpr gsl::index bits_size{8};

void string_id_serialize(const string_id& id, bit_writer& writer)
{
  writer.write({.value = gsl::narrow<uint32_t>(id.size()), .size_bits = bits_size});
  for (const char c : id) {
    writer.write({.value = gsl::narrow<uint32_t>(c), .size_bits = 8});
  }
}

string_id string_id_deserialize(bit_reader& reader)
{
  const gsl::index size{reader.read(bits_size)};
  string_id id(size, 0);

  for (gsl::index i{0}; i < size; ++i) {
    id[i] = gsl::narrow_cast<char>(reader.read(8));
  }

  return id;
}
}  // namespace eely