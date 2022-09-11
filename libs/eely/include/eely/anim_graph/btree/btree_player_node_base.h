#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/project/project.h"

namespace eely::internal {
// Base class for all blendtree player nodes.
class btree_player_node_base : public anim_graph_player_node_base {
};

// Shorter name for a unique pointer to an blendtree player node.
using btree_player_node_uptr = std::unique_ptr<btree_player_node_base>;

// Create player node from blendtree node descirption.
btree_player_node_uptr btree_player_node_create(
    const project& project,
    const anim_graph_node_base& node,
    const std::vector<btree_player_node_uptr>& player_nodes);
}  // namespace eely::internal