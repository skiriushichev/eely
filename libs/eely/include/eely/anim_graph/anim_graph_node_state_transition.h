#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely {
// Describes how a transition forms a pose when active.
enum class transition_type {
  // Frozen source state and played destination state are blended together with weight
  // continously increasing towards destination for the duration of a transition.
  frozen_fade

  // TODO: cross_fade
};

// Node that smoothes transitions between two states.
class anim_graph_node_state_transition final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_state_transition(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_state_transition(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get id of a node that checks if a transition must be started.
  [[nodiscard]] std::optional<int> get_condition_node() const;

  // Set id of a node that checks if a transition must be started.
  void set_condition_node(std::optional<int> value);

  // Get destination state node id.
  [[nodiscard]] std::optional<int> get_destination_state_node() const;

  // Set destination state node id.
  void set_destination_state_node(std::optional<int> value);

  // Get type of a transition.
  [[nodiscard]] transition_type get_transition_type() const;

  // Set type of a transition.
  void set_transition_type(transition_type value);

  // Get transition duration in seconds.
  // This is how long it takes to switch from source to destination state.
  [[nodiscard]] float get_duration_s() const;

  // Set transition duration in seconds.
  // This is how long it takes to switch from source to destination state.
  void set_duration_s(float value);

private:
  std::optional<int> _condition_node;
  std::optional<int> _destination_state_node;
  transition_type _type{transition_type::frozen_fade};
  float _duration_s{0.0F};
};

namespace internal {
static constexpr gsl::index bits_transition_type = 3;
}
}  // namespace eely