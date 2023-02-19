#include "eely/anim_graph/anim_graph_player_node_state_condition.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_state_machine.h"
#include "eely/base/assert.h"

#include <any>
#include <optional>

namespace eely::internal {
anim_graph_player_node_state_condition::anim_graph_player_node_state_condition(
    const int id,
    const std::optional<float> phase)
    : anim_graph_player_node_base{anim_graph_node_type::state_condition, id}, _phase{phase}
{
}

std::optional<float> anim_graph_player_node_state_condition::get_phase() const
{
  return _phase;
}

void anim_graph_player_node_state_condition::compute_impl(const anim_graph_player_context& context,
                                                          std::any& out_result)
{
  anim_graph_player_node_base::compute_impl(context, out_result);

  const auto* const node_state_machine = anim_graph_player_node_state_machine::get_current();
  EXPECTS(node_state_machine != nullptr);

  if (_phase.has_value()) {
    // TODO: we can also check if we reached specified phase in this exact frame,
    // but this probably hsould be a separate option.
    if (node_state_machine->get_transition_source_candidate_phase() >= _phase.value()) {
      out_result = true;
      return;
    }
  }

  out_result = false;
}
}  // namespace eely::internal