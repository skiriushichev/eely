#pragma once

#include "eely/btree/btree_node_base.h"
#include "eely/btree/btree_player_node_base.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"
#include "eely/skeleton/skeleton_pose.h"

#include <memory>
#include <optional>
#include <vector>

namespace eely {
// Runtime player for a blend tree.
class btree_player final {
public:
  // Construct a new player for specified tree and input parameters.
  explicit btree_player(const skeleton& skeleton,
                        const std::vector<btree_node_uptr>& nodes,
                        const params& params);

  // Play a blend tree and calculate pose.
  void play(float dt_s, skeleton_pose& out_pose);

private:
  std::vector<internal::btree_player_node_uptr> _player_nodes;
  internal::btree_player_node_base* _root_node;
  const params& _params;

  std::optional<uint32_t> _prev_params_version;
  internal::job_queue _job_queue;
};
}  // namespace eely