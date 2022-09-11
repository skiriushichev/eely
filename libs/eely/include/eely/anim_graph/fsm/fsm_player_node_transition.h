#pragma once

#include "eely/anim_graph/fsm/fsm_node_transition.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_queue.h"
#include "eely/job/job_restore.h"
#include "eely/job/job_save.h"

#include <optional>
#include <variant>

namespace eely::internal {
// State machine node that controls transition between two other states.
class fsm_player_node_transition final : public fsm_player_node_base {
public:
  // Construct empty node.
  fsm_player_node_transition();

  // Construct node with specified transition settings.
  fsm_player_node_transition(transition_trigger_variant trigger,
                             transition_blending blending,
                             float duration_s);

  // Return source state.
  [[nodiscard]] fsm_player_node_base* get_source() const;

  // Set source state.
  void set_source(fsm_player_node_base* source);

  // Return destination state.
  [[nodiscard]] fsm_player_node_base* get_destination() const;

  // Set destination state.
  void set_destination(fsm_player_node_base* destination);

  // Return `true` if transition should be started.
  [[nodiscard]] bool should_begin(const anim_graph_context& context);

  // Return `true` if transition is ended.
  [[nodiscard]] bool is_ended() const;

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  void on_start(const anim_graph_context& context) override;

  transition_trigger_variant _trigger;
  transition_blending _blending{transition_blending::frozen_fade};

  fsm_player_node_base* _node_source{nullptr};
  fsm_player_node_base* _node_destination{nullptr};

  internal::job_blend _blend_job;
  internal::job_save _save_job;
  internal::job_restore _restore_job;
  bool _pose_saved{false};  // `True` = there is a saved pose for a frozen fade
  std::optional<gsl::index> _saved_pose_index;
};
}  // namespace eely::internal