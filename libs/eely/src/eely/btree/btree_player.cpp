#include "eely/btree/btree_player.h"

#include "eely/btree/btree_node_base.h"
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
                           const std::vector<btree_node_uptr>& nodes,
                           const params& params)
    : _params(params), _job_queue{skeleton}
{
  using namespace eely::internal;

  EXPECTS(!nodes.empty());

  // We need to resize nodes vector in advance,
  // because some nodes are initialized with pointers to other nodes,
  // and these pointers should not be invalidated.
  _player_nodes.resize(nodes.size());

  for (gsl::index i{std::ssize(nodes) - 1}; i >= 0; --i) {
    const btree_node_uptr& node = nodes[i];
    EXPECTS(node != nullptr);

    _player_nodes[i] = btree_player_node_create(skeleton.get_project(), *node, _player_nodes);
  }

  _root_node = _player_nodes[0].get();
}

void btree_player::play(const float dt_s, skeleton_pose& out_pose)
{
  using namespace internal;

  btree_player_node_base::context context{.job_queue = _job_queue, .params = _params, .dt_s = dt_s};

  const uint32_t current_params_version{_params.get_version()};
  if (!_prev_params_version.has_value() || _prev_params_version.value() != current_params_version) {
    _prev_params_version = current_params_version;
    _root_node->on_params_changed(context);
  }

  _root_node->play(context);

  _job_queue.execute(out_pose);
}
}  // namespace eely