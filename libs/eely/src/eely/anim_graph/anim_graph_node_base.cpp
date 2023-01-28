#include "eely/anim_graph/anim_graph_node_base.h"

#include "eely/anim_graph/anim_graph_node_and.h"
#include "eely/anim_graph/anim_graph_node_blend.h"
#include "eely/anim_graph/anim_graph_node_clip.h"
#include "eely/anim_graph/anim_graph_node_param.h"
#include "eely/anim_graph/anim_graph_node_param_comparison.h"
#include "eely/anim_graph/anim_graph_node_random.h"
#include "eely/anim_graph/anim_graph_node_speed.h"
#include "eely/anim_graph/anim_graph_node_state.h"
#include "eely/anim_graph/anim_graph_node_state_condition.h"
#include "eely/anim_graph/anim_graph_node_state_machine.h"
#include "eely/anim_graph/anim_graph_node_state_transition.h"
#include "eely/anim_graph/anim_graph_node_sum.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <unordered_set>

namespace eely {
anim_graph_node_base::anim_graph_node_base(const anim_graph_node_type type, const int id)
    : _type{type}, _id{id}
{
}

anim_graph_node_base::anim_graph_node_base(const anim_graph_node_type type,
                                           internal::bit_reader& reader)
    : _type{type}, _id{internal::bit_reader_read<int>(reader, internal::bits_anim_graph_node_id)}
{
}

void anim_graph_node_base::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  // Type is serialized and read outside,
  // because we need to know type of a node to call appropriate constructor
  // (it's read in `bit_reader_read` and written in `bit_writer_writer`)

  bit_writer_write(writer, _id, bits_anim_graph_node_id);
}

anim_graph_node_type anim_graph_node_base::get_type() const
{
  return _type;
}

int anim_graph_node_base::get_id() const
{
  return _id;
}

namespace internal {
template <>
anim_graph_node_uptr bit_reader_read(bit_reader& reader)
{
  const auto type{bit_reader_read<anim_graph_node_type>(reader, bits_anim_graph_node_type)};

  switch (type) {
    case anim_graph_node_type::and_logic: {
      return std::make_unique<anim_graph_node_and>(reader);
    } break;

    case anim_graph_node_type::blend: {
      return std::make_unique<anim_graph_node_blend>(reader);
    } break;

    case anim_graph_node_type::clip: {
      return std::make_unique<anim_graph_node_clip>(reader);
    } break;

    case anim_graph_node_type::random: {
      return std::make_unique<anim_graph_node_random>(reader);
    } break;

    case anim_graph_node_type::speed: {
      return std::make_unique<anim_graph_node_speed>(reader);
    } break;

    case anim_graph_node_type::param: {
      return std::make_unique<anim_graph_node_param>(reader);
    } break;

    case anim_graph_node_type::param_comparison: {
      return std::make_unique<anim_graph_node_param_comparison>(reader);
    } break;

    case anim_graph_node_type::sum: {
      return std::make_unique<anim_graph_node_sum>(reader);
    } break;

    case anim_graph_node_type::state_condition: {
      return std::make_unique<anim_graph_node_state_condition>(reader);
    } break;

    case anim_graph_node_type::state_machine: {
      return std::make_unique<anim_graph_node_state_machine>(reader);
    } break;

    case anim_graph_node_type::state_transition: {
      return std::make_unique<anim_graph_node_state_transition>(reader);
    } break;

    case anim_graph_node_type::state: {
      return std::make_unique<anim_graph_node_state>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown anim graph node type for deserialization");
    } break;
  }
}

void bit_writer_write(bit_writer& writer, const anim_graph_node_base& node)
{
  anim_graph_node_type type{node.get_type()};

  bit_writer_write(writer, type, bits_anim_graph_node_type);
  node.serialize(writer);
}
}  // namespace internal
}  // namespace eely