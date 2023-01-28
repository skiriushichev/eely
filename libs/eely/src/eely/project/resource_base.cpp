#include "eely/project/resource_base.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

namespace eely {
resource_base::resource_base(internal::bit_reader& reader)
    : _id{internal::bit_reader_read<string_id>(reader)}
{
}

resource_base::resource_base(string_id id) : _id{std::move(id)} {}

resource_base::~resource_base() = default;

void resource_base::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  bit_writer_write(writer, _id);
}

const string_id& resource_base::get_id() const
{
  return _id;
}
}  // namespace eely