#include "eely/project/resource_base.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

namespace eely {
resource_base::resource_base(bit_reader& reader) : _id{string_id_deserialize(reader)} {}

resource_base::resource_base(string_id id) : _id{std::move(id)} {}

resource_base::~resource_base() = default;

void resource_base::serialize(bit_writer& writer) const
{
  string_id_serialize(_id, writer);
}

const string_id& resource_base::get_id() const
{
  return _id;
}
}  // namespace eely