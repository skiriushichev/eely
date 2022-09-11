#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/fsm/fsm_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
// State machine node that plays another state machine.
class fsm_node_fsm final : public fsm_node_base {
public:
  // Construct empty node.
  explicit fsm_node_fsm() = default;

  // Construct node from a memory buffer.
  explicit fsm_node_fsm(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) override;

  // Get id of a state machine to play.
  [[nodiscard]] const string_id& get_fsm_id() const;

  // Set id of a state machine to play.
  void set_fsm_id(string_id fsm_id);

private:
  string_id _fsm_id;
};
}  // namespace eely