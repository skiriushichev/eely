#include "eely/btree/btree_player.h"

#include "eely/btree/btree_player_node_base.h"
#include "eely/btree/btree_player_node_blend.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"
#include "eely/skeleton/skeleton_pose.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace eely {
btree_player::btree_player(const skeleton& skeleton,
                           std::vector<std::unique_ptr<internal::btree_player_node_base>> nodes,
                           const params& params)
    : _job_queue{skeleton}, _nodes{std::move(nodes)}, _root_node{*_nodes.back()}, _params(params)
{
  EXPECTS(!_nodes.empty());
}

void btree_player::play(const float dt_s, skeleton_pose& out_pose)
{
  internal::btree_player_node_base::context context{
      .job_queue = _job_queue, .params = _params, .dt_s = dt_s};

  const uint32_t current_params_version{_params.get_version()};
  if (!_prev_params_version.has_value() || _prev_params_version.value() != current_params_version) {
    _prev_params_version = current_params_version;
    _root_node.on_params_changed(context);
  }

  _root_node.play(context);

  _job_queue.execute(out_pose);
}
}  // namespace eely