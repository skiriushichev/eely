#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"

#include <any>
#include <optional>

namespace eely::internal {
// Runtime version of `anim_graph_node_state_condition`.
class anim_graph_player_node_state_condition final : public anim_graph_player_node_base {
public:
  // Construct node with specified state conditions.
  anim_graph_player_node_state_condition(std::optional<float> phase);

  // Return required phase value.
  [[nodiscard]] std::optional<float> get_phase() const;

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  std::optional<float> _phase;
};
}  // namespace eely::internal