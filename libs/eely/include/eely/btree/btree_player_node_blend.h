#pragma once

#include "eely/base/string_id.h"
#include "eely/btree/btree_player_node_base.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <vector>

namespace eely::internal {
// Node that blends between children based on a float parameter.
class btree_player_node_blend final : public btree_player_node_base {
public:
  // Defines a child of this node:
  // pointer to a child node and with what param value it should be played.
  struct child final {
    btree_player_node_base& node;
    float param_value{0.0F};
  };

  explicit btree_player_node_blend(string_id param_id, std::vector<child> children);

  void on_params_changed(const context& context) override;

  gsl::index play(const context& context) override;

private:
  string_id _param_id;
  std::vector<child> _children;

  float _sync_phase{0.0F};

  btree_player_node_base* _blended_child_from{nullptr};
  btree_player_node_base* _blended_child_to{nullptr};
  float _blend_weight{0.0F};

  job_blend _job;
};
}  // namespace eely::internal