#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>

namespace eely {
// Node that produces sum of two poses.
// At least one of them must be additive.
class anim_graph_node_sum final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_sum(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_sum(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get a node that produces first pose.
  [[nodiscard]] std::optional<int> get_first_node_id() const;

  // Set a node that produces first pose.
  void set_first_node_id(std::optional<int> value);

  // Get a node that produces second pose.
  [[nodiscard]] std::optional<int> get_second_node_id() const;

  // Set a node that produces second pose.
  void set_second_node_id(std::optional<int> value);

private:
  std::optional<int> _first_node;
  std::optional<int> _second_node;
};
}  // namespace eely