#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/project/project.h"

#include <gsl/util>

#include <algorithm>
#include <optional>
#include <vector>

namespace eely::internal {
// Type of a player node.
// Used as an alternative to dynamic casts for better perfomance
// in cases when specific node types are treated differently.
enum class fsm_player_node_type { btree, fsm, transition };

class fsm_player_node_transition;

// Base class for all state machine player nodes.
class fsm_player_node_base : public anim_graph_player_node_base {
public:
  // Consutrct player node with specified type.
  explicit fsm_player_node_base(fsm_player_node_type type);

  // Return type of this node.
  [[nodiscard]] fsm_player_node_type get_type() const;

  // Remember transition that has this node as a source.
  // This is used by `fsm_player` to check if any transitions from current node should be activated.
  void add_out_transition(fsm_player_node_transition* transition);

  // Get transitions that have this node as a source.
  [[nodiscard]] const std::vector<fsm_player_node_transition*>& get_out_transitions() const;

private:
  const fsm_player_node_type _type;
  std::vector<fsm_player_node_transition*> _out_transitions;
};

// Shorter name for a unique pointer to an state machine player node.
using fsm_player_node_uptr = std::unique_ptr<fsm_player_node_base>;

// Create player node from state machine node.
// After all nodes are created, `fsm_player_node_connect`
// to establish connections between transitions and their source/desitnations.
fsm_player_node_uptr fsm_player_node_create(const project& project, anim_graph_node_base& node);

// Remember source and destination nodes for each transition in a state machine.
void fsm_player_node_connect(const std::vector<anim_graph_node_uptr>& nodes,
                             const std::vector<fsm_player_node_uptr>& player_nodes);

// Implementation

inline fsm_player_node_base::fsm_player_node_base(fsm_player_node_type type) : _type{type} {}

inline fsm_player_node_type fsm_player_node_base::get_type() const
{
  return _type;
}

inline void fsm_player_node_base::add_out_transition(fsm_player_node_transition* transition)
{
  EXPECTS(std::find(_out_transitions.begin(), _out_transitions.end(), transition) ==
          _out_transitions.end());
  _out_transitions.push_back(transition);
}

inline const std::vector<fsm_player_node_transition*>& fsm_player_node_base::get_out_transitions()
    const
{
  return _out_transitions;
}
}  // namespace eely::internal