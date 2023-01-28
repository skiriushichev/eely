#include "eely/anim_graph/anim_graph_node_speed.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>

namespace eely {
anim_graph_node_speed::anim_graph_node_speed(const int id)
    : anim_graph_node_base{anim_graph_node_type::speed, id}
{
}

anim_graph_node_speed::anim_graph_node_speed(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::speed, reader}
{
  using namespace eely::internal;

  _child_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
  _speed_provider_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
}

void anim_graph_node_speed::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _child_node, bits_anim_graph_node_id);
  bit_writer_write(writer, _speed_provider_node, bits_anim_graph_node_id);
}

anim_graph_node_uptr anim_graph_node_speed::clone() const
{
  return std::make_unique<anim_graph_node_speed>(*this);
}

std::optional<int> anim_graph_node_speed::get_child_node() const
{
  return _child_node;
}

void anim_graph_node_speed::set_child_node(const std::optional<int> value)
{
  _child_node = value;
}

std::optional<int> anim_graph_node_speed::get_speed_provider_node() const
{
  return _speed_provider_node;
}

void anim_graph_node_speed::set_speed_provider_node(const std::optional<int> value)
{
  _speed_provider_node = value;
}
}  // namespace eely