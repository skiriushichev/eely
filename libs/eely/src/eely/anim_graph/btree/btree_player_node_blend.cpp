#include "eely/anim_graph/btree/btree_player_node_blend.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_queue.h"
#include "eely/math/math_utils.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <algorithm>
#include <cstdint>
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

void btree_player_node_blend::prepare(const anim_graph_context& context)
{
  // Select up to two children that this node is going to blend
  // And calculate blending coefficient
  // Only do that if param has changed since last play

  const params::data& param_data{context.params.get_data(_param_id)};
  if (param_data.version != _prev_param_version) {
    _prev_param_version = param_data.version;

    const gsl::index children_size{std::ssize(_children)};

    const float param_value{std::get<float>(param_data.value)};

    std::optional<gsl::index> upper_bound_child_index_opt;
    for (gsl::index i{0}; i < children_size; ++i) {
      const child& c{_children[i]};
      if (c.param_value >= param_value) {
        upper_bound_child_index_opt = i;
        break;
      }
    }

    // Play single node if supplied value is out of range or if its param value is exact
    // Play two nodes with weight based on difference in parameters and supplied parameter value

    if (!upper_bound_child_index_opt.has_value()) {
      // Parameter value is greater than all nodes,
      // use last node (with greatest value) as a fallback
      _blended_child_from = nullptr;
      _blended_child_to = &_children[children_size - 1].node;
      _blend_weight = 1.0F;
    }
    else {
      const gsl::index upper_bound_child_index{upper_bound_child_index_opt.value()};

      child& upper_bound_child{_children[upper_bound_child_index]};

      if (float_near(upper_bound_child.param_value, param_value)) {
        // Selected node's parameter value is the same as supplied one,
        // play this one node
        _blended_child_from = nullptr;
        _blended_child_to = &upper_bound_child.node;
        _blend_weight = 1.0F;
      }
      else {
        EXPECTS(upper_bound_child_index >= 1);
        child& lower_bound_child{_children[upper_bound_child_index - 1]};

        const float param_value_range{upper_bound_child.param_value -
                                      lower_bound_child.param_value};
        EXPECTS(!float_near(param_value_range, 0.0F));

        _blended_child_from = &lower_bound_child.node;
        _blended_child_to = &upper_bound_child.node;
        _blend_weight = (param_value - lower_bound_child.param_value) / param_value_range;
        EXPECTS(_blend_weight >= 0.0F && _blend_weight <= 1.0F);
      }
    }
  }

  // Prepare children and update this node's duration

  if (_blended_child_from == nullptr) {
    _blended_child_to->prepare(context);
    set_duration_s(_blended_child_to->get_duration_s());
  }
  else {
    _blended_child_from->prepare(context);
    _blended_child_to->prepare(context);
    set_duration_s(std::lerp(_blended_child_from->get_duration_s(),
                             _blended_child_to->get_duration_s(), _blend_weight));
  }
}

gsl::index btree_player_node_blend::enqueue_job(const anim_graph_context& context)
{
  update_phase_from_context(context);

  // This node always forces synchronized phase down the hierarchy
  // TODO: should this be an option?
  anim_graph_context context_pass_on{context};
  context_pass_on.sync_phase = get_phase();

  if (_blended_child_from == nullptr) {
    return _blended_child_to->enqueue_job(context_pass_on);
  }

  const gsl::index first_job_index{_blended_child_from->enqueue_job(context_pass_on)};
  const gsl::index second_job_index{_blended_child_to->enqueue_job(context_pass_on)};

  _job.set_first_job_index(first_job_index);
  _job.set_second_job_index(second_job_index);
  _job.set_weight(_blend_weight);
  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal