#include "eely/btree/btree_node_clip.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"

namespace eely {
btree_node_clip::btree_node_clip(bit_reader& reader)
    : btree_node_base(reader), _clip_id{string_id_deserialize(reader)}
{
}

void btree_node_clip::serialize(bit_writer& writer) const
{
  btree_node_base::serialize(writer);
  string_id_serialize(_clip_id, writer);
}

std::unique_ptr<btree_node_base> btree_node_clip::clone() const
{
  return std::make_unique<btree_node_clip>(*this);
}

void btree_node_clip::collect_dependencies(std::unordered_set<string_id>& out_dependencies)
{
  out_dependencies.insert(_clip_id);
}

void btree_node_clip::set_clip_id(string_id clip_id)
{
  _clip_id = std::move(clip_id);
}

const string_id& btree_node_clip::get_clip_id() const
{
  return _clip_id;
}
}  // namespace eely