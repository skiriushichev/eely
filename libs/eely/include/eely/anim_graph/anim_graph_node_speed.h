#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>

namespace eely {
// Node that changes playback speed of a child node (and all its descendants).
class anim_graph_node_speed final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_speed(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_speed(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get child node id.
  [[nodiscard]] std::optional<int> get_child_node() const;

  // Set child node id.
  void set_child_node(std::optional<int> value);

  // Get playback speed.
  [[nodiscard]] std::optional<int> get_speed_provider_node() const;

  // Set playback speed.
  void set_speed_provider_node(std::optional<int> value);

private:
  std::optional<int> _child_node;
  std::optional<int> _speed_provider_node;
};
}  // namespace eely