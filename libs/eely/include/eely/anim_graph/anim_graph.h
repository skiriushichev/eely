#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_uncooked.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <memory>
#include <vector>

namespace eely {
// Represents an animation graph, which is traversed to produce skeleton poses.
// Contains different types of nodes that calculate poses and combine them,
// such as playing clips, blending poses, running state machines etc.
class anim_graph final : public resource {
public:
  // Construct animation graph resource from a memory buffer.
  explicit anim_graph(const project& project, internal::bit_reader& reader);

  // Construct animation graph from an uncooked counterpart.
  explicit anim_graph(const project& project, const anim_graph_uncooked& uncooked);

  explicit anim_graph(const anim_graph& other);

  explicit anim_graph(anim_graph&& other) noexcept;

  ~anim_graph() override = default;

  anim_graph& operator=(const anim_graph& other);

  anim_graph& operator=(anim_graph&& other) noexcept;

  // Serialize animation graph into a memory buffer.
  void serialize(internal::bit_writer& writer) const override;

  // Get id of a skeleton this graph is for.
  [[nodiscard]] const string_id& get_skeleton_id() const;

  // Get list of graph nodes.
  [[nodiscard]] const std::vector<anim_graph_node_uptr>& get_nodes() const;

  // Get id of a node from which traversal starts.
  [[nodiscard]] int get_root_node_id() const;

private:
  string_id _skeleton_id;
  std::vector<anim_graph_node_uptr> _nodes;
  int _root_node_id;
};
}  // namespace eely