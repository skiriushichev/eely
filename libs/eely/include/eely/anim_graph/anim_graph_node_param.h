#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>

namespace eely {
// Node that provides value of an external parameter.
class anim_graph_node_param final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_param(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_param(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get id of a parameter.
  [[nodiscard]] const string_id& get_param_id() const;

  // Set id of a parameter.
  void set_param_id(string_id value);

private:
  string_id _param_id;
};
}  // namespace eely