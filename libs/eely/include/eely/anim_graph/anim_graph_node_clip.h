#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
// Node that plays an animation clip.
class anim_graph_node_clip final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_clip(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_clip(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) override;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;

  // Get id of a clip to play.
  [[nodiscard]] const string_id& get_clip_id() const;

  // Set id of a clip to play.
  void set_clip_id(string_id value);

private:
  string_id _clip_id;
};
}  // namespace eely