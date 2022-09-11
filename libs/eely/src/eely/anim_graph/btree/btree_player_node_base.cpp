#include "eely/anim_graph/btree/btree_player_node_base.h"

#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_node_add.h"
#include "eely/anim_graph/btree/btree_node_blend.h"
#include "eely/anim_graph/btree/btree_node_btree.h"
#include "eely/anim_graph/btree/btree_node_clip.h"
#include "eely/anim_graph/btree/btree_node_fsm.h"
#include "eely/anim_graph/btree/btree_player_node_add.h"
#include "eely/anim_graph/btree/btree_player_node_blend.h"
#include "eely/anim_graph/btree/btree_player_node_btree.h"
#include "eely/anim_graph/btree/btree_player_node_clip.h"
#include "eely/anim_graph/btree/btree_player_node_fsm.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/base/assert.h"
#include "eely/clip/clip.h"

namespace eely::internal {
btree_player_node_uptr btree_player_node_create(
    const project& project,
    const anim_graph_node_base& node,
    const std::vector<btree_player_node_uptr>& player_nodes)
{
  // TODO: use enum + polymorphic_downcast

  if (const auto* node_add{dynamic_cast<const btree_node_add*>(&node)}) {
    const std::vector<gsl::index>& children_indices{node_add->get_children_indices()};
    EXPECTS(children_indices.size() == 2);

    const gsl::index first_child_index{children_indices[0]};
    const gsl::index second_child_index{children_indices[1]};
    return std::make_unique<btree_player_node_add>(*player_nodes.at(first_child_index),
                                                   *player_nodes.at(second_child_index));
  }

  if (const auto* node_blend{dynamic_cast<const btree_node_blend*>(&node)}) {
    const std::vector<gsl::index>& children_indices{node_blend->get_children_indices()};
    EXPECTS(!children_indices.empty());

    const std::vector<float>& children_param_values{node_blend->get_children_param_values()};
    EXPECTS(std::size(children_param_values) == std::size(children_indices));

    std::vector<btree_player_node_blend::child> children;
    for (gsl::index i{0}; i < std::ssize(children_indices); ++i) {
      const gsl::index child_index{children_indices[i]};

      children.push_back(
          {.node = *player_nodes.at(child_index), .param_value = children_param_values.at(i)});
    }

    return std::make_unique<btree_player_node_blend>(node_blend->get_param_id(),
                                                     std::move(children));
  }

  if (const auto* node_btree{dynamic_cast<const btree_node_btree*>(&node)}) {
    const auto* btree{project.get_resource<eely::btree>(node_btree->get_btree_id())};
    EXPECTS(btree != nullptr);

    return std::make_unique<btree_player_node_btree>(*btree);
  }

  if (const auto* node_clip{dynamic_cast<const btree_node_clip*>(&node)}) {
    const auto* clip{project.get_resource<eely::clip>(node_clip->get_clip_id())};
    EXPECTS(clip != nullptr);

    return std::make_unique<btree_player_node_clip>(*clip);
  }

  if (const auto* node_fsm{dynamic_cast<const btree_node_fsm*>(&node)}) {
    const auto* fsm{project.get_resource<eely::fsm>(node_fsm->get_fsm_id())};
    EXPECTS(fsm != nullptr);

    return std::make_unique<btree_player_node_fsm>(*fsm);
  }

  EXPECTS(false);
  return nullptr;
}
}  // namespace eely::internal