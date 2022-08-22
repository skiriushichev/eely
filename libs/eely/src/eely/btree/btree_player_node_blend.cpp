#include "eely/btree/btree_player_node_blend.h"

#include "eely/base/string_id.h"
#include "eely/btree/btree_player_node_base.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_queue.h"
#include "eely/math/math_utils.h"
#include "eely/params/params.h"

#include <gsl/narrow>
#include <gsl/util>

#include <algorithm>
#include <optional>
#include <vector>

namespace eely::internal {
btree_player_node_blend::btree_player_node_blend(string_id param_id, std::vector<child> children)
    : _param_id{std::move(param_id)}, _children{std::move(children)}
{
  EXPECTS(!_param_id.empty());
  EXPECTS(!_children.empty());
  EXPECTS(std::is_sorted(_children.begin(), _children.end(), [](const auto& c0, const auto& c1) {
    return c0.param_value < c1.param_value;
  }));
}

void btree_player_node_blend::on_params_changed(const context& context)
{
  for (child& c : _children) {
    c.node.on_params_changed(context);
  }

  // Select up to two children that this node is going to blend
  // And calculate blending coefficient

  const gsl::index children_size{std::ssize(_children)};

  const float param_value{context.params.get_float(_param_id)};

  gsl::index upper_bound_child_index{-1};
  for (gsl::index i{0}; i < children_size; ++i) {
    const child& c{_children[i]};
    if (c.param_value >= param_value) {
      upper_bound_child_index = i;
      break;
    }
  }

  child& upper_bound_child{_children[upper_bound_child_index]};
  if (upper_bound_child_index == 0 || float_near(upper_bound_child.param_value, param_value)) {
    // Single node will be played
    _blended_child_from = nullptr;
    _blended_child_to = &upper_bound_child.node;
    _blend_weight = 1.0F;
    set_duration_s(_blended_child_to->get_duration_s());
  }
  else {
    // Two nodes will be played and then blended together

    EXPECTS(upper_bound_child_index >= 1);
    child& lower_bound_child{_children[upper_bound_child_index - 1]};

    const float param_value_range{upper_bound_child.param_value - lower_bound_child.param_value};
    EXPECTS(!float_near(param_value_range, 0.0F));

    _blended_child_from = &lower_bound_child.node;
    _blended_child_to = &upper_bound_child.node;
    _blend_weight = (param_value - lower_bound_child.param_value) / param_value_range;

    set_duration_s(std::lerp(_blended_child_from->get_duration_s(),
                             _blended_child_to->get_duration_s(), _blend_weight));
  }
}

gsl::index btree_player_node_blend::play(const context& context)
{
  btree_player_node_base::context context_pass_on{context};

  if (!context.sync_phase.has_value()) {
    // This node always forces synchronized mode for all children nodes
    // TODO: should this be an option?
    _sync_phase = std::fmod(_sync_phase + context.dt_s / get_duration_s(), 1.0F);
    context_pass_on.sync_phase = _sync_phase;
  }
  else {
    _sync_phase = context.sync_phase.value();
  }

  EXPECTS(_blended_child_to != nullptr);

  if (_blended_child_from == nullptr) {
    // Single node
    EXPECTS(float_near(_blend_weight, 1.0F));
    return _blended_child_to->play(context_pass_on);
  }

  // Two nodes

  EXPECTS(_blend_weight >= 0.0F && _blend_weight <= 1.0F);

  const gsl::index from_job_index{_blended_child_from->play(context_pass_on)};
  const gsl::index to_job_index{_blended_child_to->play(context_pass_on)};

  _job.set_dependencies({.first = from_job_index, .second = to_job_index, .weight = _blend_weight});

  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal