#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"

#include <memory>

namespace eely {
// Node that plays a clip.
class btree_node_clip final : public btree_node_base {
public:
  explicit btree_node_clip() = default;

  explicit btree_node_clip(bit_reader& reader);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] std::unique_ptr<btree_node_base> clone() const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) override;

  // Set id of a clip to play.
  void set_clip_id(string_id clip_id);

  // Get id of a clip to play.
  [[nodiscard]] const string_id& get_clip_id() const;

private:
  string_id _clip_id;
};
}  // namespace eely