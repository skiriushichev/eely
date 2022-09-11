#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/fsm/fsm_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <memory>

namespace eely {
// Describes transition trigger that happens when source state ends.
struct transition_trigger_source_end final {
};

// Describes transition trigger that happens when
// a parameter becomes equal to specified value.
struct transition_trigger_param_value final {
  string_id param_id;
  params::type_variant value;
};

// Variant for all transition trigger types.
// Trigger defines when transition activates.
using transition_trigger_variant =
    std::variant<transition_trigger_source_end, transition_trigger_param_value>;

// Describes how source and destination states are blended during a transition.
enum class transition_blending {
  // Source state and destination states are both played,
  // and are blended together while source state progressively looses its weight.
  cross_fade,

  // Source state becomes frozen and destination state is played,
  // and they're blended together while source state progressively looses its weight.
  frozen_fade
};

// State machine node that is responsible for transition between two other states.
class fsm_node_transition final : public fsm_node_base {
public:
  // Consutrct empty node.
  explicit fsm_node_transition() = default;

  // Construct node from a memory buffer.
  explicit fsm_node_transition(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;

  // Get transition's trigger data.
  [[nodiscard]] transition_trigger_variant get_trigger() const;

  // Set transition's trigger data.
  void set_trigger(const transition_trigger_variant& trigger);

  // Get transition's blending type.
  [[nodiscard]] transition_blending get_blending() const;

  // Set transition's blending type.
  void set_blending(const transition_blending& blending);

  // Get duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Set duration in seconds.
  void set_duration_s(float duration);

private:
  transition_trigger_variant _trigger;
  transition_blending _blending{transition_blending::frozen_fade};
  float _duration_s{0.0F};
};
}  // namespace eely