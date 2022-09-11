#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"
#include "eely/anim_graph/fsm/fsm_player_node_transition.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <vector>

namespace eely::internal {
// Runtime player for a state machine.
// Player serves as an entry point to the state machine,
// owning all nodes and managing transitions between states.
class fsm_player final {
public:
  // Construct empty player.
  explicit fsm_player() = default;

  // Construct player from specified state machine.
  explicit fsm_player(const fsm& fsm);

  // Reset state machine to initial state.
  void reset(const anim_graph_context& context);

  // Update state machine's state for the next `play`.
  void prepare(const anim_graph_context& context);

  // Register jobs that produce this state machine's pose.
  gsl::index enqueue_job(const anim_graph_context& context);

  // Return state machine's current state duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Return state machine's current state phase.
  [[nodiscard]] float get_phase() const;

private:
  std::vector<std::unique_ptr<internal::fsm_player_node_base>> _player_nodes;

  internal::fsm_player_node_base* _current_node{nullptr};
};
}  // namespace eely::internal