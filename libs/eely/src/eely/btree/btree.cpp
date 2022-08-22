#include "eely/btree/btree.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"
#include "eely/btree/btree_player.h"
#include "eely/btree/btree_uncooked.h"
#include "eely/params/params.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <gsl/narrow>
#include <gsl/util>

#include <vector>

namespace eely {
static void collect_children_recursive_impl(const gsl::index original_node_index,
                                            const std::vector<btree_node_uptr>& original_nodes,
                                            const gsl::index new_node_index,
                                            std::vector<btree_node_uptr>& new_nodes)
{
  const btree_node_uptr& original_node{original_nodes[original_node_index]};
  EXPECTS(original_node != nullptr);

  btree_node_uptr& new_node{new_nodes[new_node_index]};
  EXPECTS(new_node != nullptr);

  const std::vector<gsl::index>& original_node_children_indices{
      original_node->get_children_indices()};

  std::vector<gsl::index>& new_node_children_indices{new_node->get_children_indices()};

  // We need to reassign children nodes indices,
  // since they might differ from the original list.
  // So first clone children and assign new indices,
  // and then do the same recursively for each child.

  new_node_children_indices.clear();

  for (const gsl::index original_child_node_index : original_node_children_indices) {
    const btree_node_uptr& original_child_node{original_nodes[original_child_node_index]};

    EXPECTS(new_nodes.capacity() >= new_nodes.size() + 1);
    new_nodes.push_back(original_child_node->clone());

    new_node_children_indices.push_back(std::ssize(new_nodes) - 1);
  }

  for (gsl::index i{0}; i < std::ssize(original_node_children_indices); ++i) {
    const gsl::index new_child_node_index{new_node_children_indices[i]};
    const gsl::index original_child_node_index{original_node_children_indices[i]};
    collect_children_recursive_impl(original_child_node_index, original_nodes, new_child_node_index,
                                    new_nodes);
  }
}

// Traverse tree from a specified root node, and assemble a new list of nodes.
// This list has stricter requirements on order of nodes and also omits unused nodes.
// Nodes in a result list satisfy these requirements:
//  - children always come after their parent
//  - all direct children of a node are always next to each other
// This allows for a more cache-friendly traversal by a `btree_player`.
static std::vector<btree_node_uptr> collect_children(const gsl::index root_node_index,
                                                     const std::vector<btree_node_uptr>& nodes)
{
  std::vector<btree_node_uptr> result;

  // We need to reserve memory upfront,
  // so that pointers are not invalidated when we push elements.
  result.reserve(nodes.size());

  result.push_back(nodes[root_node_index]->clone());
  collect_children_recursive_impl(root_node_index, nodes, 0, result);

  return result;
}

btree::btree(const project& project, bit_reader& reader) : resource(project, reader)
{
  using namespace eely::internal;

  const gsl::index nodes_size{reader.read(bits_btree_node_index)};
  _nodes.reserve(nodes_size);

  for (gsl::index i{0}; i < nodes_size; ++i) {
    _nodes.push_back(btree_node_deserialize(reader));
  }

  _skeleton_id = string_id_deserialize(reader);
}

btree::btree(const project& project, const btree_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  using namespace eely::internal;

  EXPECTS(!uncooked.get_nodes().empty());

  const gsl::index root_node_index{uncooked.get_root_node_index().value_or(0)};
  _nodes = collect_children(root_node_index, uncooked.get_nodes());

  _skeleton_id = uncooked.get_skeleton_id();
}

void btree::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  const gsl::index nodes_size{std::ssize(_nodes)};
  writer.write({.value = gsl::narrow<uint32_t>(nodes_size), .size_bits = bits_btree_node_index});

  for (gsl::index i{0}; i < nodes_size; ++i) {
    EXPECTS(_nodes[i] != nullptr);
    btree_node_serialize(*_nodes[i], writer);
  }

  string_id_serialize(_skeleton_id, writer);
}

std::unique_ptr<btree_player> btree::create_player(const params& params) const
{
  const project& project{get_project()};
  const skeleton* skeleton{project.get_resource<eely::skeleton>(_skeleton_id)};
  EXPECTS(skeleton != nullptr);

  return std::make_unique<btree_player>(*skeleton, _nodes, params);
}
}  // namespace eely