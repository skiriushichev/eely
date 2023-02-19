#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_state_transition.h"

#include <any>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_state`.
class anim_graph_player_node_state final : public anim_graph_player_node_pose_base {
public:
  // Construct node with specified name.
  // The rest of the data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_state(int id, string_id name);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Get a node that calculates pose for this state.
  [[nodiscard]] anim_graph_player_node_pose_base* get_pose_node() const;

  // Set a node that calculates pose for this state.
  void set_pose_node(anim_graph_player_node_pose_base* pose_node);

  // Get list of transitions from this state.
  [[nodiscard]] const std::vector<anim_graph_player_node_state_transition*>& get_out_transitions()
      const;

  // Set list of transitions from this state.
  // This will also initialize state's breakpoints.
  void set_out_transitions(std::vector<anim_graph_player_node_state_transition*> transitions);

  // Update breakpoints.
  // Needs to be called once out transitions are set and these transitions are fully initialized.
  void update_breakpoints();

  // Get list of state's breakpoints.
  // Breakpoint is a phase at which a transition can happen from this state.
  // E.g. if we have a transition at phase 0.5, and we have time delta enough to go
  // from 0.48 to 0.52, state must only move to 0.5
  // and the rest of the time delta should be handled by the transition.
  [[nodiscard]] const std::vector<float>& get_breakpoints() const;

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  string_id _name;
  anim_graph_player_node_pose_base* _pose_node{nullptr};
  std::vector<anim_graph_player_node_state_transition*> _out_transition_nodes;
  std::vector<float> _breakpoints;
};
}  // namespace eely::internal