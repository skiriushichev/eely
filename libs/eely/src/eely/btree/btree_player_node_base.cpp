#include "eely/btree/btree_player_node_base.h"

#include "eely/btree/btree_node_add.h"
#include "eely/btree/btree_node_blend.h"
#include "eely/btree/btree_node_clip.h"
#include "eely/btree/btree_player_node_add.h"
#include "eely/btree/btree_player_node_blend.h"
#include "eely/btree/btree_player_node_clip.h"
#include "eely/clip/clip.h"

namespace eely::internal {
btree_player_node_uptr btree_player_node_create(const project& project,
                                                btree_node_base& node,
                                                std::vector<btree_player_node_uptr>& player_nodes)
{
  if (const auto* node_add{dynamic_cast<const btree_node_add*>(&node)}) {
    auto first{node_add->get_children_indices()[0]};
    auto second{node_add->get_children_indices()[1]};
    return std::make_unique<btree_player_node_add>(*player_nodes[first], *player_nodes[second]);
  }

  if (const auto* node_blend{dynamic_cast<const btree_node_blend*>(&node)}) {
    std::vector<btree_player_node_blend::child> children;
    for (gsl::index i{0}; i < (gsl::index)node_blend->get_children_indices().size(); ++i) {
      children.push_back({.node = *player_nodes[node_blend->get_children_indices()[i]],
                          .param_value = node_blend->get_children_param_values()[i]});
    }

    return std::make_unique<btree_player_node_blend>(node_blend->get_param_id(), children);
  }

  if (const auto* node_clip{dynamic_cast<const btree_node_clip*>(&node)}) {
    const auto* clip{project.get_resource<eely::clip>(node_clip->get_clip_id())};
    return std::make_unique<btree_player_node_clip>(*clip);
  }

  return nullptr;
}
}  // namespace eely::internal