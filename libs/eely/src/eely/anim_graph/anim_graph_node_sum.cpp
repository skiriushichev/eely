#include "eely/anim_graph/anim_graph_node_sum.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>

namespace eely {
anim_graph_node_sum::anim_graph_node_sum(const int id)
    : anim_graph_node_base{anim_graph_node_type::sum, id}
{
}

anim_graph_node_sum::anim_graph_node_sum(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::sum, reader},
      _first_node{bit_reader_read<std::optional<int>>(reader, internal::bits_anim_graph_node_id)},
      _second_node{bit_reader_read<std::optional<int>>(reader, internal::bits_anim_graph_node_id)}
{
}

void anim_graph_node_sum::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _first_node, bits_anim_graph_node_id);
  bit_writer_write(writer, _second_node, bits_anim_graph_node_id);
}

std::unique_ptr<anim_graph_node_base> anim_graph_node_sum::clone() const
{
  return std::make_unique<anim_graph_node_sum>(*this);
}

std::optional<int> anim_graph_node_sum::get_first_node_id() const
{
  return _first_node;
}

void anim_graph_node_sum::set_first_node_id(const std::optional<int> value)
{
  _first_node = value;
}

std::optional<int> anim_graph_node_sum::get_second_node_id() const
{
  return _second_node;
}

void anim_graph_node_sum::set_second_node_id(const std::optional<int> value)
{
  _second_node = value;
}
}  // namespace eely