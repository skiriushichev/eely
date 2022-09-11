#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/btree/btree_node_base.h"

#include <memory>

namespace eely {
// Blendtree node that produces sum of poses from children nodes.
class btree_node_add final : public btree_node_base {
public:
  using btree_node_base::btree_node_base;

  [[nodiscard]] std::unique_ptr<anim_graph_node_base> clone() const override;
};
}  // namespace eely