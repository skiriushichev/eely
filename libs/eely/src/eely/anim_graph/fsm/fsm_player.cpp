#include "eely/anim_graph/fsm/fsm_player.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"
#include "eely/anim_graph/fsm/fsm_player_node_transition.h"
#include "eely/base/base_utils.h"
#include "eely/job/job_queue.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <vector>

namespace eely::internal {
fsm_player::fsm_player(const fsm& fsm)
{
  using namespace eely::internal;

  const project& project{fsm.get_project()};
  const std::vector<anim_graph_node_uptr>& nodes{fsm.get_nodes()};

  EXPECTS(!nodes.empty());

  // We need to resize nodes vector in advance,
  // because some nodes are initialized with pointers to other nodes,
  // and these pointers should not be invalidated.
  _player_nodes.resize(nodes.size());

  for (gsl::index i{0}; i < std::ssize(nodes); ++i) {
    const anim_graph_node_uptr& node = nodes[i];
    EXPECTS(node != nullptr);

    _player_nodes[i] = fsm_player_node_create(project, *nodes[i]);
  }

  fsm_player_node_connect(nodes, _player_nodes);

  EXPECTS(!_player_nodes.empty());
  _current_node = _player_nodes[0].get();
}

void fsm_player::reset(const anim_graph_context& /*context*/)
{
  EXPECTS(!_player_nodes.empty());
  _current_node = _player_nodes[0].get();
}

void fsm_player::prepare(const anim_graph_context& context)
{
  using namespace eely::internal;

  EXPECTS(_current_node != nullptr);

  // Process nodes in a loop in case there are multiple transitions that should begin and end in a
  // single frame (e.g. zero duration transitions based on parameter values).
  while (true) {
    // Check if we need to start transition from current node
    for (fsm_player_node_transition* transition : _current_node->get_out_transitions()) {
      if (transition->should_begin(context)) {
        _current_node = transition;
      }
    }

    _current_node->prepare(context);

    // Check if we need to end current transition (if we're in one)
    if (_current_node->get_type() == fsm_player_node_type::transition) {
      auto* transition{polymorphic_downcast<fsm_player_node_transition*>(_current_node)};
      if (transition->is_ended()) {
        _current_node = transition->get_destination();
        continue;
      }
    }

    break;
  }
}

gsl::index fsm_player::enqueue_job(const anim_graph_context& context)
{
  EXPECTS(_current_node != nullptr);
  return _current_node->enqueue_job(context);
}

float fsm_player::get_duration_s() const
{
  EXPECTS(_current_node != nullptr);
  return _current_node->get_duration_s();
}

float fsm_player::get_phase() const
{
  EXPECTS(_current_node != nullptr);
  return _current_node->get_phase();
}
}  // namespace eely::internal