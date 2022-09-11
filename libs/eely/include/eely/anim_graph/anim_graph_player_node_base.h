#pragma once

#include "eely/base/assert.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <optional>

namespace eely {
// Execution context for animation graph.
struct anim_graph_context final {
  // Number of graph's plays made so far.
  uint32_t play_counter{1};

  // Queue to feed jobs into by graph nodes.
  internal::job_queue& job_queue;

  // External parameters that control the graph.
  const params& params;

  // How much seconds has passed since the last update.
  float dt_s{0.0F};

  // If set, its value forces phase for all nodes below.
  // Used to synchronize animations based on their normalized phase.
  std::optional<float> sync_phase;
};

namespace internal {
// Base class for animation graph runtime nodes.
// These nodes can be played to produces poses.
class anim_graph_player_node_base {
public:
  virtual ~anim_graph_player_node_base() = default;

  // Update node's state and state of all of its relevant children.
  // This should always be called before `play` (which only registers the jobs).
  // Updating a node is split into these two methods, because when
  // synchronizing children, we need to know their durations in order to give them
  // a synchronized phase (which is passed into `play`'s context).
  virtual void prepare(const anim_graph_context& /*context*/);

  // Register jobs that produce this node's pose.
  virtual gsl::index enqueue_job(const anim_graph_context& context) = 0;

  // Return this node's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Return this node's normalized phase.
  [[nodiscard]] float get_phase() const;

protected:
  // Set this node's duration in seconds.
  void set_duration_s(float duration_s);

  // Set this node's phase manually.
  void set_phase(float phase);

  // Update phase from the context.
  // If synchornized phase is given, it will be used,
  // otherwise phase will be increased based on time delta.
  // If `allow_wrap` is `true`, phase will wrap over 1.0F value,
  // otherwise it will be clamped to 1.0F.
  void update_phase_from_context(const anim_graph_context& context, bool allow_wrap = true);

  virtual void on_start(const anim_graph_context& context);

private:
  uint32_t _prev_graph_play_counter{0};

  float _duration_s{0.0F};
  float _phase{0.0F};
};

// Implementation

inline void anim_graph_player_node_base::set_duration_s(const float duration_s)
{
  EXPECTS(duration_s >= 0.0F);
  _duration_s = duration_s;
}

inline float anim_graph_player_node_base::get_duration_s() const
{
  return _duration_s;
}

inline void anim_graph_player_node_base::set_phase(const float phase)
{
  EXPECTS(phase >= 0.0F && phase <= 1.0F);
  _phase = phase;
}

inline float anim_graph_player_node_base::get_phase() const
{
  return _phase;
}
}  // namespace internal
}  // namespace eely