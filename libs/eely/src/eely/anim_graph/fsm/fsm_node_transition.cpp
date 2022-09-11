#include "eely/anim_graph/fsm/fsm_node_transition.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/fsm/fsm_node_base.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <stdexcept>

namespace eely {
namespace internal {
constexpr gsl::index bits_transition_trigger_type = 1;
constexpr gsl::index bits_transition_blending = 1;
}  // namespace internal

fsm_node_transition::fsm_node_transition(bit_reader& reader) : fsm_node_base(reader)
{
  using namespace eely::internal;

  const gsl::index trigger_index{reader.read(bits_transition_trigger_type)};
  if (trigger_index == 0) {
    _trigger = transition_trigger_source_end{};
  }
  else if (trigger_index == 1) {
    transition_trigger_param_value trigger;
    trigger.param_id = string_id_deserialize(reader);
    trigger.value = params_value_deserialize(reader);
    _trigger = trigger;
  }
  else {
    throw std::runtime_error{"Invalid variant index type for transition trigger."};
  }

  _blending = static_cast<transition_blending>(reader.read(bits_transition_blending));

  _duration_s = bit_cast<float>(reader.read(32));
}

void fsm_node_transition::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  fsm_node_base::serialize(writer);

  writer.write({.value = gsl::narrow<uint32_t>(_trigger.index()),
                .size_bits = bits_transition_trigger_type});

  if (const auto* trigger{std::get_if<transition_trigger_param_value>(&_trigger)}) {
    string_id_serialize(trigger->param_id, writer);
    params_value_serialize(trigger->value, writer);
  }

  writer.write({.value = static_cast<uint32_t>(_blending), .size_bits = bits_transition_blending});

  writer.write({.value = bit_cast<uint32_t>(_duration_s), .size_bits = 32});
}

std::unique_ptr<anim_graph_node_base> fsm_node_transition::clone() const
{
  return std::make_unique<fsm_node_transition>(*this);
}

transition_trigger_variant fsm_node_transition::get_trigger() const
{
  return _trigger;
}

void fsm_node_transition::set_trigger(const transition_trigger_variant& trigger)
{
  _trigger = trigger;
}

transition_blending fsm_node_transition::get_blending() const
{
  return _blending;
}

void fsm_node_transition::set_blending(const transition_blending& blending)
{
  _blending = blending;
}

float fsm_node_transition::get_duration_s() const
{
  return _duration_s;
}

void fsm_node_transition::set_duration_s(const float duration_s)
{
  _duration_s = duration_s;
}
}  // namespace eely