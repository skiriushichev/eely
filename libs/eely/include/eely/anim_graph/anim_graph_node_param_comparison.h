#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <memory>

namespace eely {
// Node that compares external parameter value with provided one.
class anim_graph_node_param_comparison final : public anim_graph_node_base {
public:
  // Operation to use for comparison.
  enum class op { equal, not_equal };

  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_param_comparison(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_param_comparison(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Return id of a parameter to compare.
  [[nodiscard]] const string_id& get_param_id() const;

  // Set id of a parameter to compare.
  void set_param_id(string_id value);

  // Get value to compare parameter with.
  [[nodiscard]] const param_value& get_value() const;

  // Set value to compare parameter with.
  void set_value(const param_value& value);

  // Get operation used for comparison.
  [[nodiscard]] op get_op() const;

  // Set operation used for comparison.
  void set_op(op value);

private:
  string_id _param_id;
  param_value _value;
  op _op{op::equal};
};

namespace internal {
static constexpr gsl::index bits_op_type = 3;
}
}  // namespace eely