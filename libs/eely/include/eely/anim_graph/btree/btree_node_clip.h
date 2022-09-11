#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/btree/btree_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
// Blendtree node that plays an animation clip.
class btree_node_clip final : public btree_node_base {
public:
  // Construct empty node.
  explicit btree_node_clip() = default;

  // Construct node from a memory buffer.
  explicit btree_node_clip(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) override;

  // Get id of a clip to play.
  [[nodiscard]] const string_id& get_clip_id() const;

  // Set id of a clip to play.
  void set_clip_id(string_id clip_id);

private:
  string_id _clip_id;
};
}  // namespace eely