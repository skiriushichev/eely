#include "eely/anim_graph/anim_graph_node_and.h"

#include "eely/anim_graph/anim_graph_node_base.h"

#include <memory>
#include <vector>

namespace eely {
anim_graph_node_and::anim_graph_node_and(const int id)
    : anim_graph_node_base{anim_graph_node_type::and_logic, id}
{
}

anim_graph_node_and::anim_graph_node_and(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::and_logic, reader}
{
  using namespace eely::internal;

  _children_nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_children_nodes); ++i) {
    _children_nodes[i] = bit_reader_read<int>(reader, bits_anim_graph_node_id);
  }
}

void anim_graph_node_and::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _children_nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_children_nodes); ++i) {
    bit_writer_write(writer, _children_nodes[i], bits_anim_graph_node_id);
  }
}

std::unique_ptr<anim_graph_node_base> anim_graph_node_and::clone() const
{
  return std::make_unique<anim_graph_node_and>(*this);
}

const std::vector<int>& anim_graph_node_and::get_children_nodes() const
{
  return _children_nodes;
}

std::vector<int>& anim_graph_node_and::get_children_nodes()
{
  return _children_nodes;
}
}  // namespace eely