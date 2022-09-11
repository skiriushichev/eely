#include "eely/anim_graph/fsm/fsm_player_node_fsm.h"

#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
fsm_player_node_fsm::fsm_player_node_fsm() : fsm_player_node_base(fsm_player_node_type::fsm) {}

fsm_player_node_fsm::fsm_player_node_fsm(const fsm& fsm)
    : fsm_player_node_base(fsm_player_node_type::fsm), _fsm_player(fsm)
{
}

void fsm_player_node_fsm::prepare(const anim_graph_context& context)
{
  fsm_player_node_base::prepare(context);

  _fsm_player.prepare(context);
  set_duration_s(_fsm_player.get_duration_s());
}

gsl::index fsm_player_node_fsm::enqueue_job(const anim_graph_context& context)
{
  gsl::index result{_fsm_player.enqueue_job(context)};
  set_phase(_fsm_player.get_phase());
  return result;
}

void fsm_player_node_fsm::on_start(const anim_graph_context& context)
{
  _fsm_player.reset(context);
}
}  // namespace eely::internal