#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"

#include <memory>
#include <vector>

namespace eely {
// Node that blends between two children poses.
// Children are selected based on `float` parameter value.
class btree_node_blend final : public btree_node_base {
public:
  explicit btree_node_blend() = default;

  explicit btree_node_blend(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<btree_node_base> clone() const override;

  // Get id of a parameter based on which children are selected.
  [[nodiscard]] const string_id& get_param_id() const;

  // Set id of a parameter based on which children are selected.
  void set_param_id(string_id param_id);

  // Get read-only parameter values that correspond to children
  // i'th value corresponds to i'th child.
  [[nodiscard]] const std::vector<float>& get_children_param_values() const;

  // Get parameter values that correspond to children.
  // i'th value corresponds to i'th child.
  [[nodiscard]] std::vector<float>& get_children_param_values();

private:
  string_id _param_id;
  std::vector<float> _children_param_values;
};
}  // namespace eely