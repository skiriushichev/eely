#include "eely/anim_graph/anim_graph_node_clip.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
anim_graph_node_clip::anim_graph_node_clip(int id)
    : anim_graph_node_base{anim_graph_node_type::clip, id}
{
}

anim_graph_node_clip::anim_graph_node_clip(internal::bit_reader& reader)
    : anim_graph_node_base{anim_graph_node_type::clip, reader}
{
  using namespace eely::internal;

  _clip_id = bit_reader_read<string_id>(reader);
}

void anim_graph_node_clip::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  anim_graph_node_base::serialize(writer);

  bit_writer_write(writer, _clip_id);
}

void anim_graph_node_clip::collect_dependencies(std::unordered_set<string_id>& out_dependencies)
{
  out_dependencies.insert(_clip_id);
}

anim_graph_node_uptr anim_graph_node_clip::clone() const
{
  return std::make_unique<anim_graph_node_clip>(*this);
}

const string_id& anim_graph_node_clip::get_clip_id() const
{
  return _clip_id;
}

void anim_graph_node_clip::set_clip_id(string_id value)
{
  _clip_id = std::move(value);
}
}  // namespace eely