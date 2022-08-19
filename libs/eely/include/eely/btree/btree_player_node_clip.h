#pragma once

#include "eely/btree/btree_player_node_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_play.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely::internal {
// Node that plays a clip.
class btree_player_node_clip final : public btree_player_node_base {
public:
  explicit btree_player_node_clip(const clip& clip);

  gsl::index play(const context& context) override;

private:
  std::unique_ptr<clip_player_base> _player;

  float _player_time_s{0.0F};
  internal::job_play _job;
};
}  // namespace eely::internal