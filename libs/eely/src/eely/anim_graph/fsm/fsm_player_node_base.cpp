#include "eely/anim_graph/fsm/fsm_player_node_base.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_node_btree.h"
#include "eely/anim_graph/fsm/fsm_node_fsm.h"
#include "eely/anim_graph/fsm/fsm_node_transition.h"
#include "eely/anim_graph/fsm/fsm_player_node_btree.h"
#include "eely/anim_graph/fsm/fsm_player_node_fsm.h"
#include "eely/anim_graph/fsm/fsm_player_node_transition.h"
#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/project/project.h"

#include <gsl/util>

#include <optional>
#include <vector>

namespace eely::internal {
fsm_player_node_uptr fsm_player_node_create(const project& project, anim_graph_node_base& node)
{
  using namespace eely::internal;

  // TODO: use enum + polymorphic_downcast

  fsm_player_node_uptr result;

  if (const auto* node_btree{dynamic_cast<const fsm_node_btree*>(&node)}) {
    const btree* btree{project.get_resource<eely::btree>(node_btree->get_btree_id())};
    EXPECTS(btree != nullptr);

    return std::make_unique<fsm_player_node_btree>(*btree);
  }

  if (const auto* node_fsm{dynamic_cast<const fsm_node_fsm*>(&node)}) {
    const fsm* fsm{project.get_resource<eely::fsm>(node_fsm->get_fsm_id())};
    EXPECTS(fsm != nullptr);

    return std::make_unique<fsm_player_node_fsm>(*fsm);
  }

  if (const auto* node_transition{dynamic_cast<const fsm_node_transition*>(&node)}) {
    auto node_player_transition = std::make_unique<fsm_player_node_transition>(
        node_transition->get_trigger(), node_transition->get_blending(),
        node_transition->get_duration_s());

    return std::move(node_player_transition);
  }

  EXPECTS(false);
  return nullptr;
}

void fsm_player_node_connect(const std::vector<anim_graph_node_uptr>& nodes,
                             const std::vector<fsm_player_node_uptr>& player_nodes)
{
  // Since state machine is represented as a graph,
  // transition node is a child of another node (which is a transition's source),
  // and transition's child is its destination,
  // i.e. SOURCE NODE -> TRANSITION NODE -> DESTINATION NODE.
  // We need to remember source and destination nodes in a transition node for runtime,
  // as well as pointers to all out transitions for a state,
  // i.e. SOURCE NODE <-> TRANSITION NODE -> DESTINATION NODE.

  EXPECTS(nodes.size() == player_nodes.size());

  const gsl::index nodes_size{std::ssize(nodes)};

  for (gsl::index node_index{0}; node_index < nodes_size; ++node_index) {
    const anim_graph_node_uptr& node{nodes[node_index]};

    const std::vector<gsl::index>& children_indices{node->get_children_indices()};
    if (children_indices.empty()) {
      continue;
    }

    const fsm_player_node_uptr& player_node{player_nodes[node_index]};

    const bool is_transition{player_node->get_type() == fsm_player_node_type::transition};
    if (is_transition) {
      // Transition can only have a single destination.
      EXPECTS(children_indices.size() == 1);
      const gsl::index destination_index{children_indices[0]};

      const fsm_player_node_uptr& player_node_destination{player_nodes[destination_index]};

      auto* player_node_transition{
          polymorphic_downcast<fsm_player_node_transition*>(player_node.get())};

      player_node_transition->set_destination(player_node_destination.get());
    }
    else {
      // A node can have multiple out transitions.
      for (const gsl::index child_index : children_indices) {
        auto* player_node_transition{
            polymorphic_downcast<fsm_player_node_transition*>(player_nodes[child_index].get())};

        player_node->add_out_transition(player_node_transition);
        player_node_transition->set_source(player_node.get());
      }
    }
  }
}
}  // namespace eely::internal