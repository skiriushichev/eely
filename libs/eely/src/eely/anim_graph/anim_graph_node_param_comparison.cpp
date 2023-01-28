#include "eely/anim_graph/anim_graph_node_param_comparison.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <memory>

namespace eely {
anim_graph_node_param_comparison::anim_graph_node_param_comparison(int id)
    : anim_graph_node_base{anim_graph_node_type::param_comparison, id}
{
}

anim_graph_node_param_comparison::anim_graph_node_param_comparison(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::param_comparison, reader}
{
  using namespace eely::internal;

  _param_id = bit_reader_read<string_id>(reader);
  _value = bit_reader_read<param_value>(reader);
  _op = bit_reader_read<op>(reader, bits_op_type);
}

void anim_graph_node_param_comparison::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _param_id);
  bit_writer_write(writer, _value);
  bit_writer_write(writer, _op, bits_op_type);
}

anim_graph_node_uptr anim_graph_node_param_comparison::clone() const
{
  return std::make_unique<anim_graph_node_param_comparison>(*this);
}

const string_id& anim_graph_node_param_comparison::get_param_id() const
{
  return _param_id;
}

void anim_graph_node_param_comparison::set_param_id(string_id value)
{
  _param_id = std::move(value);
}

const param_value& anim_graph_node_param_comparison::get_value() const
{
  return _value;
}

void anim_graph_node_param_comparison::set_value(const param_value& value)
{
  _value = value;
}

anim_graph_node_param_comparison::op anim_graph_node_param_comparison::get_op() const
{
  return _op;
}

void anim_graph_node_param_comparison::set_op(const op value)
{
  _op = value;
}
}  // namespace eely