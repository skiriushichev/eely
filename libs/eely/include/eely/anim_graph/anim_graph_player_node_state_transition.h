#pragma once

#include "eely/anim_graph/anim_graph_node_state_transition.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_restore.h"
#include "eely/job/job_save.h"

#include <any>
#include <array>

namespace eely::internal {
// Forward declaration to cut out cyclic dependencies for compilation
// (transition stores pointers to states, and states store pointers to transitions).
class anim_graph_player_node_state;

// Runtime version of `anim_graph_node_state_transition`.
class anim_graph_player_node_state_transition final : public anim_graph_player_node_pose_base {
public:
  // Construct transition node with specified type and duration.
  // The rest of the data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_state_transition(int id, transition_type type, float duration_s);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Return `true` if all conditions for this transition are satisfied
  // and transition can be initiated.
  [[nodiscard]] bool conditions_are_satisfied(const anim_graph_player_context& context) const;

  // Return `true` if this transition will be finished at specified phase,
  // i.e. state machine can just switch to the destination state.
  [[nodiscard]] bool is_finished(float phase) const;

  // Get node that checks conditions for this transition.
  [[nodiscard]] const anim_graph_player_node_base* get_condition_node() const;

  // Set node that checks conditions for this transition.
  void set_condition_node(anim_graph_player_node_base* node);

  // Get state this transition is set up to go to in a graph.
  // Note: source state is not kept in a transition node,
  // since multiple states can go to the same destination state via one transition.
  [[nodiscard]] anim_graph_player_node_state* get_original_destination_state_node() const;

  // Get state this transition is set up to go to in a graph.
  // Note: source state is not kept in a transition node,
  // since multiple states can go to the same destination state via one transition.
  void set_original_destination_state_node(anim_graph_player_node_state* node);

  // Get state this transitions is currently going to.
  // This might differ from original destination if transition was reversed.
  [[nodiscard]] anim_graph_player_node_state* get_current_destination_state_node();

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  [[maybe_unused]] transition_type _type;  // Will be used once other transition types are added
  anim_graph_player_node_base* _condition_node{nullptr};
  anim_graph_player_node_state* _destination_state_node{nullptr};

  bool _reversed{false};
  anim_graph_player_node_state* _current_source{nullptr};
  anim_graph_player_node_state* _current_destination{nullptr};

  bool _saved_pose_slots_acquired{false};
  std::array<gsl::index, 2> _saved_pose_slots;
  gsl::index _saved_pose_source_slot_index{0};
  float _saved_pose_source_phase{0.0F};

  job_save _save_source_state_job;
  job_save _save_transition_job;
  job_restore _restore_job;
  job_blend _blend_job;
};
}  // namespace eely::internal