#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_play.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely::internal {
class btree_player_node_clip final : public btree_player_node_base {
public:
  explicit btree_player_node_clip(const clip& clip);

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  std::unique_ptr<clip_player_base> _player;

  internal::job_play _job;
};
}  // namespace eely::internal