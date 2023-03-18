#include "eely/anim_graph/anim_graph_node_state_transition.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely {
anim_graph_node_state_transition::anim_graph_node_state_transition(const int id)
    : anim_graph_node_base{anim_graph_node_type::state_transition, id}
{
}

anim_graph_node_state_transition::anim_graph_node_state_transition(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::state_transition, reader}
{
  using namespace eely::internal;

  _condition_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
  _destination_state_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
  _type = bit_reader_read<transition_type>(reader, bits_transition_type);
  _duration_s = bit_reader_read<float>(reader);
  _reversible = bit_reader_read<bool>(reader);
}

void anim_graph_node_state_transition::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _condition_node, bits_anim_graph_node_id);
  bit_writer_write(writer, _destination_state_node, bits_anim_graph_node_id);
  bit_writer_write(writer, _type, bits_transition_type);
  bit_writer_write(writer, _duration_s);
  bit_writer_write(writer, _reversible);
}

anim_graph_node_uptr anim_graph_node_state_transition::clone() const
{
  return std::make_unique<anim_graph_node_state_transition>(*this);
}

std::optional<int> anim_graph_node_state_transition::get_condition_node() const
{
  return _condition_node;
}

void anim_graph_node_state_transition::set_condition_node(const std::optional<int> value)
{
  _condition_node = value;
}

std::optional<int> anim_graph_node_state_transition::get_destination_state_node() const
{
  return _destination_state_node;
}

void anim_graph_node_state_transition::set_destination_state_node(const std::optional<int> value)
{
  _destination_state_node = value;
}

transition_type anim_graph_node_state_transition::get_transition_type() const
{
  return _type;
}

void anim_graph_node_state_transition::set_transition_type(const transition_type value)
{
  _type = value;
}

float anim_graph_node_state_transition::get_duration_s() const
{
  return _duration_s;
}

void anim_graph_node_state_transition::set_duration_s(const float value)
{
  _duration_s = value;
}

bool anim_graph_node_state_transition::get_reversible() const
{
  return _reversible;
}

void anim_graph_node_state_transition::set_reversible(bool value)
{
  _reversible = value;
}
}  // namespace eely