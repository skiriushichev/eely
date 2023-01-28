#include "eely/anim_graph/anim_graph_player_node_blend.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/base/assert.h"
#include "eely/job/job_blend.h"

#include <gsl/util>

#include <algorithm>
#include <any>
#include <optional>
#include <vector>

namespace eely::internal {
anim_graph_player_node_blend::anim_graph_player_node_blend()
    : anim_graph_player_node_pose_base{anim_graph_node_type::blend}
{
}

void anim_graph_player_node_blend::update_duration(const anim_graph_player_context& context)
{
  anim_graph_player_node_pose_base::update_duration(context);

  select_blended_nodes(context);

  if (_blended_node_from == nullptr) {
    _blended_node_to->update_duration(context);
    set_duration_s(_blended_node_to->get_duration_s());
  }
  else {
    _blended_node_from->update_duration(context);
    _blended_node_to->update_duration(context);
    set_duration_s(std::lerp(_blended_node_from->get_duration_s(),
                             _blended_node_to->get_duration_s(), _blend_weight));
  }
}

void anim_graph_player_node_blend::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  for (const pose_node_data& pose_node : _pose_nodes) {
    out_descendants.push_back(&pose_node.node);
    pose_node.node.collect_descendants(out_descendants);
  }

  if (_factor_node != nullptr) {
    out_descendants.push_back(_factor_node);
    _factor_node->collect_descendants(out_descendants);
  }
}

const std::vector<anim_graph_player_node_blend::pose_node_data>&
anim_graph_player_node_blend::get_pose_nodes() const
{
  return _pose_nodes;
}

std::vector<anim_graph_player_node_blend::pose_node_data>&
anim_graph_player_node_blend::get_pose_nodes()
{
  return _pose_nodes;
}

anim_graph_player_node_base* anim_graph_player_node_blend::get_factor_node() const
{
  return _factor_node;
}

void anim_graph_player_node_blend::set_factor_node(anim_graph_player_node_base* node)
{
  _factor_node = node;
}

void anim_graph_player_node_blend::compute_impl(const anim_graph_player_context& context,
                                                std::any& out_result)
{
  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  const float next_phase_unwrapped{get_next_phase_unwrapped(context)};

  apply_next_phase(context);

  // This node always forces synchronized phase down the hierarchy
  // TODO: should this be an option?
  anim_graph_player_context context_pass_on{context};
  context_pass_on.sync_enabled = true;

  // Pass unwrapped phase on purpose here.
  // So that children nodes know if we moved pass 1.0F
  // without remembering previous phase value.
  context_pass_on.sync_phase = next_phase_unwrapped;

  if (_blended_node_from == nullptr) {
    out_result = _blended_node_to->compute(context_pass_on);
    return;
  }

  const auto first_job_index{
      std::any_cast<gsl::index>(_blended_node_from->compute(context_pass_on))};
  const auto second_job_index{
      std::any_cast<gsl::index>(_blended_node_to->compute(context_pass_on))};

  _job_blend.set_first_job_index(first_job_index);
  _job_blend.set_second_job_index(second_job_index);
  _job_blend.set_weight(_blend_weight);
  out_result = context.job_queue.add_job(_job_blend);
}

void anim_graph_player_node_blend::select_blended_nodes(const anim_graph_player_context& context)
{
  // Select up to two children that this node is going to blend
  // And calculate blending coefficient
  // Only do that if param has changed since last play

  EXPECTS(!_pose_nodes.empty());
  EXPECTS(std::is_sorted(_pose_nodes.begin(), _pose_nodes.end(),
                         [](const auto& n0, const auto& n1) { return n0.factor < n1.factor; }));
  EXPECTS(_factor_node != nullptr);

  const float factor{std::any_cast<float>(_factor_node->compute(context))};

  if (factor == _prev_factor) {
    return;
  }

  _prev_factor = factor;

  const gsl::index children_size{std::ssize(_pose_nodes)};

  std::optional<gsl::index> upper_bound_child_index_opt;
  for (gsl::index i{0}; i < children_size; ++i) {
    const pose_node_data& n{_pose_nodes[i]};
    if (n.factor >= factor) {
      upper_bound_child_index_opt = i;
      break;
    }
  }

  // Play single node if supplied value is out of range or if its param value is exact
  // Play two nodes with weight based on difference in parameters and supplied parameter value

  if (!upper_bound_child_index_opt.has_value()) {
    // Parameter value is greater than all nodes,
    // use last node (with greatest value) as a fallback
    _blended_node_from = nullptr;
    _blended_node_to = &_pose_nodes[children_size - 1].node;
    _blend_weight = 1.0F;
  }
  else {
    const gsl::index upper_bound_child_index{upper_bound_child_index_opt.value()};

    pose_node_data& upper_bound_node{_pose_nodes[upper_bound_child_index]};

    if (float_near(upper_bound_node.factor, factor)) {
      // Selected node's parameter value is the same as supplied one,
      // play this one node
      _blended_node_from = nullptr;
      _blended_node_to = &upper_bound_node.node;
      _blend_weight = 1.0F;
    }
    else {
      EXPECTS(upper_bound_child_index >= 1);
      pose_node_data& lower_bound_node{_pose_nodes[upper_bound_child_index - 1]};

      const float factor_range{upper_bound_node.factor - lower_bound_node.factor};
      EXPECTS(!float_near(factor_range, 0.0F));

      _blended_node_from = &lower_bound_node.node;
      _blended_node_to = &upper_bound_node.node;
      _blend_weight = (factor - lower_bound_node.factor) / factor_range;
      EXPECTS(_blend_weight >= 0.0F && _blend_weight <= 1.0F);
    }
  }
}
}  // namespace eely::internal