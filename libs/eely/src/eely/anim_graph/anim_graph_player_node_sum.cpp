#include "eely/anim_graph/anim_graph_player_node_sum.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/job/job_add.h"

#include <gsl/util>

#include <any>
#include <vector>

namespace eely::internal {
anim_graph_player_node_sum::anim_graph_player_node_sum()
    : anim_graph_player_node_pose_base{anim_graph_node_type::sum}
{
}

void anim_graph_player_node_sum::update_duration(const anim_graph_player_context& context)
{
  EXPECTS(_first_node != nullptr);
  EXPECTS(_second_node != nullptr);

  anim_graph_player_node_pose_base::update_duration(context);

  _first_node->update_duration(context);
  _second_node->update_duration(context);
  set_duration_s(std::max(_first_node->get_duration_s(), _second_node->get_duration_s()));
}

void anim_graph_player_node_sum::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  if (_first_node != nullptr) {
    out_descendants.push_back(_first_node);
    _first_node->collect_descendants(out_descendants);
  }

  if (_second_node != nullptr) {
    out_descendants.push_back(_second_node);
    _second_node->collect_descendants(out_descendants);
  }
}

anim_graph_player_node_pose_base* anim_graph_player_node_sum::get_first_node() const
{
  return _first_node;
}

void anim_graph_player_node_sum::set_first_node(anim_graph_player_node_pose_base* const node)
{
  _first_node = node;
}

anim_graph_player_node_pose_base* anim_graph_player_node_sum::get_second_node() const
{
  return _second_node;
}

void anim_graph_player_node_sum::set_second_node(anim_graph_player_node_pose_base* const node)
{
  _second_node = node;
}

void anim_graph_player_node_sum::compute_impl(const anim_graph_player_context& context,
                                              std::any& out_result)
{
  EXPECTS(_first_node != nullptr);
  EXPECTS(_second_node != nullptr);

  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  apply_next_phase(context);

  const auto first_job_index{std::any_cast<gsl::index>(_first_node->compute(context))};
  const auto second_job_index{std::any_cast<gsl::index>(_second_node->compute(context))};

  _job_add.set_first_job_index(first_job_index);
  _job_add.set_second_job_index(second_job_index);
  out_result = context.job_queue.add_job(_job_add);
}
}  // namespace eely::internal