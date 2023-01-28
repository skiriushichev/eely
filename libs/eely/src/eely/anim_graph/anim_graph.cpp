#include "eely/anim_graph/anim_graph.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player.h"
#include "eely/anim_graph/anim_graph_uncooked.h"
#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <memory>
#include <vector>

namespace eely {
anim_graph::anim_graph(const project& project, internal::bit_reader& reader)
    : resource{project, reader}
{
  using namespace eely::internal;

  _skeleton_id = bit_reader_read<string_id>(reader);

  _nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_nodes); ++i) {
    _nodes[i] = bit_reader_read<anim_graph_node_uptr>(reader);
  }

  _root_node_id = bit_reader_read<int>(reader, bits_anim_graph_node_id);
}

anim_graph::anim_graph(const project& project, const anim_graph_uncooked& uncooked)
    : resource{project, uncooked.get_id()}
{
  _skeleton_id = uncooked.get_skeleton_id();

  _nodes.reserve(uncooked.get_nodes().size());
  for (const auto& node : uncooked.get_nodes()) {
    _nodes.push_back(node->clone());
  }

  EXPECTS(!_nodes.empty());
  _root_node_id = uncooked.get_root_node_id().value_or(_nodes[0]->get_id());
}

anim_graph::anim_graph(const anim_graph& other) : resource{other.get_project(), other.get_id()}
{
  _skeleton_id = other._skeleton_id;

  _nodes.reserve(other._nodes.size());
  for (const auto& node : other._nodes) {
    _nodes.push_back(node->clone());
  }

  _root_node_id = other._root_node_id;
}

anim_graph::anim_graph(anim_graph&& other) noexcept : resource{other.get_project(), other.get_id()}
{
  _skeleton_id = std::move(other._skeleton_id);
  _nodes = std::move(other._nodes);
  _root_node_id = other._root_node_id;
}

anim_graph& anim_graph::operator=(const anim_graph& other)
{
  if (this != &other) {
    _skeleton_id = other._skeleton_id;

    _nodes.clear();
    _nodes.reserve(other._nodes.size());
    for (const auto& node : other._nodes) {
      _nodes.push_back(node->clone());
    }

    _root_node_id = other._root_node_id;
  }

  return *this;
}

anim_graph& anim_graph::operator=(anim_graph&& other) noexcept
{
  if (this != &other) {
    _skeleton_id = std::move(other._skeleton_id);
    _nodes = std::move(other._nodes);
    _root_node_id = other._root_node_id;
  }

  return *this;
}

void anim_graph::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  bit_writer_write(writer, _skeleton_id);

  bit_writer_write(writer, _nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_nodes); ++i) {
    const anim_graph_node_uptr& node{_nodes[i]};
    if (node == nullptr) {
      continue;
    }

    bit_writer_write(writer, *node);
  }

  bit_writer_write(writer, _root_node_id, bits_anim_graph_node_id);
}

const string_id& anim_graph::get_skeleton_id() const
{
  return _skeleton_id;
}

const std::vector<anim_graph_node_uptr>& anim_graph::get_nodes() const
{
  return _nodes;
}

int anim_graph::get_root_node_id() const
{
  return _root_node_id;
}
}  // namespace eely