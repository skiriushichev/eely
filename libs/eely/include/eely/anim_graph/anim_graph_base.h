#pragma once

#include "eely/anim_graph/anim_graph_base_uncooked.h"
#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <vector>

namespace eely {
// Base cooked version for animation graph resources: blendtrees and state machines.
// Both of them can be represented a a graph of nodes,
// so it's convenvient to have a base class with common functionality.
class anim_graph_base : public resource {
public:
  // Construct resource from a memory buffer.
  explicit anim_graph_base(const project& project, bit_reader& reader);

  // Construct resource from its uncooked counterpart.
  explicit anim_graph_base(const project& project, const anim_graph_base_uncooked& uncooked);

  void serialize(bit_writer& writer) const override;

  // Return list of nodes in a graph resource.
  [[nodiscard]] const std::vector<anim_graph_node_uptr>& get_nodes() const;

  // Return skeleton id this graph resource is for.
  [[nodiscard]] const string_id& get_skeleton_id() const;

private:
  std::vector<anim_graph_node_uptr> _nodes;
  string_id _skeleton_id;
};
}  // namespace eely