#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/fsm/fsm_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
// State machine node that plays a blendtree.
class fsm_node_btree final : public fsm_node_base {
public:
  // Construct empty node.
  explicit fsm_node_btree() = default;

  // Construct node from a memory buffer.
  explicit fsm_node_btree(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) override;

  // Get id of a blendtree to play.
  [[nodiscard]] const string_id& get_btree_id() const;

  // Set id of a blendtree to play.
  void set_btree_id(string_id btree_id);

private:
  string_id _btree_id;
};
}  // namespace eely