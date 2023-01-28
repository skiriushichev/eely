#include "eely/base/string_id.h"

#include "eely/base/bit_writer.h"

#include <gsl/util>

namespace eely::internal {
void bit_writer_write(bit_writer& writer, const string_id& id)
{
  bit_writer_write(writer, id.size(), bits_string_id_size);
  for (const char c : id) {
    bit_writer_write(writer, c);
  }
}
}  // namespace eely::internal