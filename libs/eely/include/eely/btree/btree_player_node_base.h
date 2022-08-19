#pragma once

#include "eely/job/job_queue.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
// Describes base functionality of a blend tree node used by a player.
// A node can be seen as an animation, and how this animation is produced
// depends on a node: can be a clip, multiple clips blended together based on parameters, IK etc.
// Thus it has similiar properties associated with it: duration etc.
class btree_player_node_base {
public:
  // Context in which a node is executed.
  struct context final {
    // Queue to put tasks into.
    job_queue& job_queue;

    // Parameters bound to a player.
    const params& params;

    // Time that has passed since last play.
    float dt_s{0.0F};

    // If node is invoked in a synchronized mode,
    // this is the phase value in [0; 1] range.
    std::optional<float> sync_phase;
  };

  virtual ~btree_player_node_base() = default;

  // Invoked whenever parameters, bound to the blend tree, are changed.
  // Used to recalculate node state (e.g. which path it takes, what's the duration etc.).
  virtual void on_params_changed(const context& context) {}

  // Play the node and return index of a root job.
  // This results into a job(s) queued for execution,
  // which in turn will calculate the final pose.
  virtual gsl::index play(const context& context) = 0;

  // Return this node's durations in seconds.
  [[nodiscard]] float get_duration_s() const;

protected:
  // Set this node's duration in seconds.
  void set_duration_s(float duration_s);

private:
  float _duration_s{0.0F};
};

// Implementation

inline float btree_player_node_base::get_duration_s() const
{
  return _duration_s;
}

inline void btree_player_node_base::set_duration_s(const float duration_s)
{
  _duration_s = duration_s;
}
}  // namespace eely::internal