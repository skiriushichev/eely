#include "eely/anim_graph/anim_graph_node_param.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>

namespace eely {
anim_graph_node_param::anim_graph_node_param(const int id)
    : anim_graph_node_base{anim_graph_node_type::param, id}
{
}

anim_graph_node_param::anim_graph_node_param(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::param, reader}
{
  using namespace eely::internal;

  _param_id = bit_reader_read<string_id>(reader);
}

void anim_graph_node_param::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _param_id);
}

anim_graph_node_uptr anim_graph_node_param::clone() const
{
  return std::make_unique<anim_graph_node_param>(*this);
}

const string_id& anim_graph_node_param::get_param_id() const
{
  return _param_id;
}

void anim_graph_node_param::set_param_id(string_id value)
{
  _param_id = std::move(value);
}
}  // namespace eely