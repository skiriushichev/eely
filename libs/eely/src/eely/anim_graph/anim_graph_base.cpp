#include "eely/anim_graph/anim_graph_base.h"

#include "eely/anim_graph/anim_graph_base_uncooked.h"
#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <gsl/narrow>
#include <gsl/util>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eely {
static void collect_children_recursive_impl(
    const gsl::index original_node_index,
    const std::vector<anim_graph_node_uptr>& original_nodes,
    const gsl::index new_node_index,
    std::vector<anim_graph_node_uptr>& new_nodes,
    std::unordered_map<gsl::index, gsl::index>& original_node_to_new_node,
    std::unordered_set<gsl::index>& visited_original_nodes)
{
  if (visited_original_nodes.contains(original_node_index)) {
    // We already processed this node due to a cycle, skip it.
    return;
  }

  visited_original_nodes.insert(original_node_index);

  const anim_graph_node_uptr& original_node{original_nodes[original_node_index]};
  EXPECTS(original_node != nullptr);

  anim_graph_node_uptr& new_node{new_nodes[new_node_index]};
  EXPECTS(new_node != nullptr);

  const std::vector<gsl::index>& original_node_children_indices{
      original_node->get_children_indices()};

  std::vector<gsl::index>& new_node_children_indices{new_node->get_children_indices()};

  // We need to reassign children nodes indices,
  // since they might differ from the original list.
  // So first clone children and assign new indices,
  // and then do the same recursively for each child.

  new_node_children_indices.clear();

  for (gsl::index i{0}; i < std::ssize(original_node_children_indices); ++i) {
    const gsl::index original_child_node_index{original_node_children_indices[i]};

    if (original_node_to_new_node.contains(original_child_node_index)) {
      // This node has already been created.
      // This can happen if there is a cycle in a graph.
      // Blendtrees do not have cycles, but state machines do.
      new_node_children_indices.push_back(original_node_to_new_node[original_child_node_index]);
      continue;
    }

    const anim_graph_node_uptr& original_child_node{original_nodes[original_child_node_index]};

    EXPECTS(new_nodes.capacity() >= new_nodes.size() + 1);
    new_nodes.push_back(original_child_node->clone());

    const gsl::index new_child_node_index{std::ssize(new_nodes) - 1};
    new_node_children_indices.push_back(new_child_node_index);
    original_node_to_new_node[original_child_node_index] = new_child_node_index;
  }

  for (gsl::index i{0}; i < std::ssize(new_node_children_indices); ++i) {
    const gsl::index new_child_node_index{new_node_children_indices[i]};

    // We need to get original index of this node.
    // Since we already have mapping from original index to new index,
    // and all values are unique, just use this mapping in reverse.
    std::optional<gsl::index> original_child_node_index;
    for (const auto& kvp : original_node_to_new_node) {
      if (kvp.second == new_child_node_index) {
        original_child_node_index = kvp.first;
      }
    }
    EXPECTS(original_child_node_index.has_value());

    collect_children_recursive_impl(original_child_node_index.value(), original_nodes,
                                    new_child_node_index, new_nodes, original_node_to_new_node,
                                    visited_original_nodes);
  }
}

static std::vector<anim_graph_node_uptr> collect_children(
    const gsl::index root_node_index,
    const std::vector<anim_graph_node_uptr>& nodes)
{
  std::vector<anim_graph_node_uptr> result;

  // We need to reserve memory upfront,
  // so that pointers are not invalidated when we push elements,
  // since nodes reference their children via pointers.
  result.reserve(nodes.size());

  std::unordered_map<gsl::index, gsl::index> original_node_to_new_node;
  std::unordered_set<gsl::index> visited_original_nodes;

  result.push_back(nodes[root_node_index]->clone());
  original_node_to_new_node[root_node_index] = 0;

  collect_children_recursive_impl(root_node_index, nodes, 0, result, original_node_to_new_node,
                                  visited_original_nodes);

  return result;
}

anim_graph_base::anim_graph_base(const project& project, bit_reader& reader)
    : resource(project, reader)
{
  using namespace eely::internal;

  const gsl::index nodes_size{reader.read(bits_anim_graph_node_index)};
  _nodes.reserve(nodes_size);

  for (gsl::index i{0}; i < nodes_size; ++i) {
    _nodes.push_back(anim_graph_node_deserialize(reader));
  }

  _skeleton_id = string_id_deserialize(reader);
}

anim_graph_base::anim_graph_base(const project& project, const anim_graph_base_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  using namespace eely::internal;

  EXPECTS(!uncooked.get_nodes().empty());

  // Traverse tree from a specified root node, and assemble a new list of nodes.
  // This list has stricter requirements on order of nodes and also omits unused ones.
  // Nodes in a result list satisfy these requirements:
  //  - children always come after their parent
  //  - all direct children of a node are always next to each other
  // This allows for a more cache-friendly traversal by the players.
  // TODO: state machines probably do need nodes that are not reachable from the root.
  const gsl::index root_node_index{uncooked.get_root_node_index().value_or(0)};
  _nodes = collect_children(root_node_index, uncooked.get_nodes());

  _skeleton_id = uncooked.get_skeleton_id();
}

void anim_graph_base::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  const gsl::index nodes_size{std::ssize(_nodes)};
  writer.write(
      {.value = gsl::narrow<uint32_t>(nodes_size), .size_bits = bits_anim_graph_node_index});

  for (const anim_graph_node_uptr& node : _nodes) {
    EXPECTS(node != nullptr);
    anim_graph_node_serialize(*node, writer);
  }

  string_id_serialize(_skeleton_id, writer);
}

const std::vector<anim_graph_node_uptr>& anim_graph_base::get_nodes() const
{
  return _nodes;
}

const string_id& anim_graph_base::get_skeleton_id() const
{
  return _skeleton_id;
}
}  // namespace eely