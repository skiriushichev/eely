#include "eely/anim_graph/anim_graph_node_state_machine.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <vector>

namespace eely {
anim_graph_node_state_machine::anim_graph_node_state_machine(const int id)
    : anim_graph_node_base{anim_graph_node_type::state_machine, id}
{
}

anim_graph_node_state_machine::anim_graph_node_state_machine(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::state_machine, reader}
{
  using namespace eely::internal;

  _state_nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_state_nodes); ++i) {
    _state_nodes[i] = bit_reader_read<int>(reader, bits_anim_graph_node_id);
  }
}

void anim_graph_node_state_machine::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _state_nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_state_nodes); ++i) {
    bit_writer_write(writer, _state_nodes[i], bits_anim_graph_node_id);
  }
}

anim_graph_node_uptr anim_graph_node_state_machine::clone() const
{
  return std::make_unique<anim_graph_node_state_machine>(*this);
}

const std::vector<int>& anim_graph_node_state_machine::get_state_nodes() const
{
  return _state_nodes;
}

std::vector<int>& anim_graph_node_state_machine::get_state_nodes()
{
  return _state_nodes;
}
}  // namespace eely