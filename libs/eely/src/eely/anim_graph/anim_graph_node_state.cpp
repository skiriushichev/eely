#include "eely/anim_graph/anim_graph_node_state.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <optional>
#include <vector>

namespace eely {
anim_graph_node_state::anim_graph_node_state(const int id)
    : anim_graph_node_base{anim_graph_node_type::state, id}
{
}

anim_graph_node_state::anim_graph_node_state(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::state, reader}
{
  using namespace eely::internal;

  _name = bit_reader_read<string_id>(reader);

  _pose_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);

  _out_transition_nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_out_transition_nodes); ++i) {
    _out_transition_nodes[i] = bit_reader_read<int>(reader, bits_anim_graph_node_id);
  }
}

void anim_graph_node_state::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _name);

  bit_writer_write(writer, _pose_node, bits_anim_graph_node_id);

  bit_writer_write(writer, _out_transition_nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_out_transition_nodes); ++i) {
    bit_writer_write(writer, _out_transition_nodes[i], bits_anim_graph_node_id);
  }
}

anim_graph_node_uptr anim_graph_node_state::clone() const
{
  return std::make_unique<anim_graph_node_state>(*this);
}

const string_id& anim_graph_node_state::get_name() const
{
  return _name;
}

void anim_graph_node_state::set_name(string_id name)
{
  _name = std::move(name);
}

std::optional<int> anim_graph_node_state::get_pose_node() const
{
  return _pose_node;
}

void anim_graph_node_state::set_pose_node(const std::optional<int> value)
{
  _pose_node = value;
}

const std::vector<int>& anim_graph_node_state::get_out_transition_nodes() const
{
  return _out_transition_nodes;
}

std::vector<int>& anim_graph_node_state::get_out_transition_nodes()
{
  return _out_transition_nodes;
}
}  // namespace eely