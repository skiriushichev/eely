#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/job/job_blend.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <cstdint>
#include <vector>

namespace eely::internal {
class btree_player_node_blend final : public btree_player_node_base {
public:
  struct child final {
    btree_player_node_base& node;
    float param_value{0.0F};
  };

  explicit btree_player_node_blend(string_id param_id, std::vector<child> children);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  string_id _param_id;
  std::vector<child> _children;

  uint8_t _prev_param_version{0};
  btree_player_node_base* _blended_child_from{nullptr};
  btree_player_node_base* _blended_child_to{nullptr};
  float _blend_weight{0.0F};

  job_blend _job;
};
}  // namespace eely::internal