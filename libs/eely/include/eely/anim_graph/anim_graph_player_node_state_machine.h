#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_state.h"

#include <any>
#include <stack>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_state_machine`.
class anim_graph_player_node_state_machine final : public anim_graph_player_node_pose_base {
public:
  // Return currently running state machine.
  // Can be used by children nodes during their updates
  // to query information about current states etc.
  static const anim_graph_player_node_state_machine* get_current();

  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_state_machine(int id);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Set list of state nodes.
  void set_state_nodes(std::vector<anim_graph_player_node_state*> state_nodes);

  // Return state which is currently being tested as a transition source.
  [[nodiscard]] const anim_graph_player_node_state* get_transition_source_candidate() const;

  // Return phase of a current candidate state that is being tested as a transition source.
  [[nodiscard]] float get_transition_source_candidate_phase() const;

  // Return state from which a transition is happening right now.
  // This is needed because there can be multiple sources for a transition.
  [[nodiscard]] anim_graph_player_node_state* get_transition_source() const;

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  // Update current state: start new transitions, handle finished ones etc.
  // Return `true` if state has changed.
  bool update_state(const anim_graph_player_context& context, float pending_phase);

  void update_phase_copy_source();

  // Contains pointer to a state machine being updated.
  // Stack is needed because state machines can be nested.
  //
  // Disable warning on purpose.
  // This seems to be the easiest way to give access to the current state machine.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static std::stack<anim_graph_player_node_state_machine*> active_state_machines_stack;

  std::vector<anim_graph_player_node_state*> _state_nodes;

  anim_graph_player_node_pose_base* _current_node{nullptr};
  const anim_graph_player_node_state* _transition_source_candidate{nullptr};
  float _transition_source_candidate_phase{0.0F};
  anim_graph_player_node_state* _transition_source{nullptr};
};
}  // namespace eely::internal