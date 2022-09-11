#include "eely/anim_graph/btree/btree_player_node_add.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_node_add.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/job/job_add.h"
#include "eely/job/job_queue.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
btree_player_node_add::btree_player_node_add(btree_player_node_base& first,
                                             btree_player_node_base& second)
    : _first_child{first}, _second_child{second}
{
}

void btree_player_node_add::prepare(const anim_graph_context& context)
{
  _first_child.prepare(context);
  _second_child.prepare(context);

  set_duration_s(std::max(_first_child.get_duration_s(), _second_child.get_duration_s()));
}

gsl::index btree_player_node_add::enqueue_job(const anim_graph_context& context)
{
  update_phase_from_context(context);

  const gsl::index first_job_index{_first_child.enqueue_job(context)};
  const gsl::index second_job_index{_second_child.enqueue_job(context)};

  _job.set_first_job_index(first_job_index);
  _job.set_second_job_index(second_job_index);

  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal