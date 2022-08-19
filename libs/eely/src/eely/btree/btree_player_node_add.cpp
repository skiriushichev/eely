#include "eely/btree/btree_player_node_add.h"

#include "eely/btree/btree_player_node_base.h"
#include "eely/job/job_add.h"
#include "eely/job/job_queue.h"

#include <gsl/util>

#include <vector>

namespace eely::internal {
btree_player_node_add::btree_player_node_add(btree_player_node_base& first,
                                             btree_player_node_base& second)
    : _first{first}, _second{second}
{
}

void btree_player_node_add::on_params_changed(const context& context)
{
  _first.on_params_changed(context);
  _second.on_params_changed(context);
}

gsl::index btree_player_node_add::play(const context& context)
{
  const gsl::index first_job_index{_first.play(context)};
  const gsl::index second_job_index{_second.play(context)};

  _job.set_dependencies({.first = first_job_index, .second = second_job_index});

  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal