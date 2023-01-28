#include "eely/anim_graph/anim_graph_node_state_condition.h"

#include "eely/anim_graph/anim_graph_node_base.h"

#include <memory>
#include <optional>

namespace eely {
anim_graph_node_state_condition::anim_graph_node_state_condition(const int id)
    : anim_graph_node_base{anim_graph_node_type::state_condition, id}
{
}

anim_graph_node_state_condition::anim_graph_node_state_condition(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::state_condition, reader}
{
  using namespace eely::internal;

  _phase = bit_reader_read<std::optional<float>>(reader);
}

void anim_graph_node_state_condition::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _phase);
}

std::unique_ptr<anim_graph_node_base> anim_graph_node_state_condition::clone() const
{
  return std::make_unique<anim_graph_node_state_condition>(*this);
}

std::optional<float> anim_graph_node_state_condition::get_phase() const
{
  return _phase;
}

void anim_graph_node_state_condition::set_phase(std::optional<float> value)
{
  _phase = value;
}
}  // namespace eely