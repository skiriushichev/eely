#include "eely/anim_graph/anim_graph_node_blend.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>
#include <vector>

namespace eely {
anim_graph_node_blend::anim_graph_node_blend(const int id)
    : anim_graph_node_base{anim_graph_node_type::blend, id}
{
}

anim_graph_node_blend::anim_graph_node_blend(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::blend, reader}
{
  using namespace eely::internal;

  _pose_nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_pose_nodes); ++i) {
    pose_node_data& data{_pose_nodes[i]};

    data.id = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
    data.factor = bit_reader_read<float>(reader);
  }

  _factor_node = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
}

void anim_graph_node_blend::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _pose_nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_pose_nodes); ++i) {
    const pose_node_data& data{_pose_nodes[i]};
    bit_writer_write(writer, data.id, bits_anim_graph_node_id);
    bit_writer_write(writer, data.factor);
  }

  bit_writer_write(writer, _factor_node, bits_anim_graph_node_id);
}

anim_graph_node_uptr anim_graph_node_blend::clone() const
{
  return std::make_unique<anim_graph_node_blend>(*this);
}

const std::vector<anim_graph_node_blend::pose_node_data>& anim_graph_node_blend::get_pose_nodes()
    const
{
  return _pose_nodes;
}

std::vector<anim_graph_node_blend::pose_node_data>& anim_graph_node_blend::get_pose_nodes()
{
  return _pose_nodes;
}

std::optional<int> anim_graph_node_blend::get_factor_node_id() const
{
  return _factor_node;
}

void anim_graph_node_blend::set_factor_node_id(const std::optional<int> id)
{
  _factor_node = id;
}
}  // namespace eely