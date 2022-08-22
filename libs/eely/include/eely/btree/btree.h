#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/btree/btree_node_base.h"
#include "eely/btree/btree_player.h"
#include "eely/btree/btree_uncooked.h"
#include "eely/params/params.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

namespace eely {
// Represents a blend tree resource.
// Blend trees describe how animations are combined in a final pose based on input parameters.
// Animations are not necessarily clips, but also IK, physics etc.
class btree final : public resource {
public:
  // Construct blend tree from a memory buffer.
  explicit btree(const project& project, bit_reader& reader);

  // Construct blend tree from its uncooked counterpart.
  explicit btree(const project& project, const btree_uncooked& uncooked);

  void serialize(bit_writer& writer) const override;

  // Create player for this blend tree.
  [[nodiscard]] std::unique_ptr<btree_player> create_player(const params& params) const;

private:
  std::vector<btree_node_uptr> _nodes;
  string_id _skeleton_id;
};
}  // namespace eely