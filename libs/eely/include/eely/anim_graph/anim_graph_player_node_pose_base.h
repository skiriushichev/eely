#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/assert.h"

namespace eely::internal {
// Base class for graph nodes that produce pose jobs.
// Pose nodes can be seen as animations, thus they have duration and phase.
class anim_graph_player_node_pose_base : public anim_graph_player_node_base {
public:
  // Construct player pose node with specified type.
  explicit anim_graph_player_node_pose_base(anim_graph_node_type type);

  // Update this node's duration.
  // This method can be called before `compute` by parent nodes,
  // if needed.
  virtual void update_duration(const anim_graph_player_context& context);

  // Return this node's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Return this node's normalized phase.
  [[nodiscard]] float get_phase() const;

  // Return phase that this node will move to on next `compute` call, but omitting normalization.
  // This can be used to make decisions in parent nodes depending on the next phases of children.
  // E.g. a state machine can check that a current transition will be finished on this frame,
  // thus state machine can move to the state it is transitioning into without any extra work.
  float get_next_phase_unwrapped(const anim_graph_player_context& context) const;

protected:
  // Describes how phase is changed during this node's work.
  enum phase_rules {
    // Phase is copied from another node.
    copy = 1 << 0,

    // Phase wraps around 1.0F,
    // e.g. 1.1F becomes 0.1F.
    wrap = 1 << 1,

    // Phase obeys the sync mode,
    // i.e. it uses phase given by a node that initiated the sync.
    sync = 1 << 2,

    // Phase moves in reversed direction,
    // i.e. from 1.0F to 0.0F.
    reversed = 1 << 3
  };

  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

  // Set this node's duration in seconds.
  void set_duration_s(float duration_s);

  // Applies normalized value from `get_next_phase`,
  // i.e. calculates and applies phase based on specified rules for this node.
  void apply_next_phase(const anim_graph_player_context& context);

  // Set rules by which this node's phase update works.
  void set_phase_rules(int rules);

  // Change specific phase rule flag.
  void set_phase_rule(phase_rules rule, bool value);

  // Set node to copy phase from, if `phase_rules::copy` is set.
  void set_phase_copy_source(const anim_graph_player_node_pose_base* source);

private:
  float _duration_s{0.0F};
  float _phase{0.0F};

  // By default, phase is updated based on node's duration and time delta,
  // it wraps around 1.0F and respects the sync mode.
  int _phase_rules{phase_rules::wrap | phase_rules::sync};
  const anim_graph_player_node_pose_base* _phase_copy_source{nullptr};

  // Last index of a graph play at which duration of this node was updated.
  std::optional<int> _last_duration_update_play_counter;
};

// Implementation

inline float anim_graph_player_node_pose_base::get_duration_s() const
{
  return _duration_s;
}

inline float anim_graph_player_node_pose_base::get_phase() const
{
  return _phase;
}

inline void anim_graph_player_node_pose_base::set_duration_s(const float duration_s)
{
  EXPECTS(duration_s >= 0.0F);
  _duration_s = duration_s;
}

inline void anim_graph_player_node_pose_base::set_phase_copy_source(
    const anim_graph_player_node_pose_base* source)
{
  _phase_copy_source = source;
}

inline void anim_graph_player_node_pose_base::set_phase_rules(const int rules)
{
  _phase_rules = rules;
}

inline void anim_graph_player_node_pose_base::set_phase_rule(phase_rules rule, bool value)
{
  if (value) {
    set_phase_rules(_phase_rules | rule);
  }
  else {
    set_phase_rules(_phase_rules & ~rule);
  }
}
}  // namespace eely::internal