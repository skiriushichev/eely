#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/btree/btree_node_base.h"

#include <memory>

namespace eely {
// Node that produces sum of poses from children nodes.
class btree_node_add final : public btree_node_base {
public:
  using btree_node_base::btree_node_base;

  [[nodiscard]] std::unique_ptr<btree_node_base> clone() const override;
};
}  // namespace eely