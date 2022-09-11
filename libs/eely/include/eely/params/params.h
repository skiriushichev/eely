#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <cstdint>
#include <unordered_map>
#include <variant>

namespace eely {
// Describes external parameters that are used as inputs to a blendtree or a state machine.
// TODO: assign indexes to params and use them instead of map lookups.
struct params final {
public:
  using type_variant = std::variant<float, bool>;

  struct data {
    type_variant value{};
    uint8_t version{1};
  };

  // Get parameter value.
  [[nodiscard]] type_variant get(const string_id& id) const;

  // Get parameter value and metadata.
  [[nodiscard]] const data& get_data(const string_id& id) const;

  // Set parameter value.
  void set(const string_id& id, const type_variant& value);

  // Get float parameter value.
  [[nodiscard]] float get_float(const string_id& id) const;

  // Get bool parameter value.
  [[nodiscard]] bool get_bool(const string_id& id) const;

private:
  // Mutable to intiailize default values in const getters.
  mutable std::unordered_map<string_id, data> _parameters;
};

namespace internal {
// Serialize parameter value into a memory buffer.
// It then can be deserialized via `params_value_deserialize`.
void params_value_serialize(const params::type_variant& value, bit_writer& writer);

// Deserialize parameter value previously written via `params_value_serialize`.
params::type_variant params_value_deserialize(bit_reader& reader);
}  // namespace internal

// Implementation

// Get parameter value.
inline params::type_variant params::get(const string_id& id) const
{
  return _parameters[id].value;
}

// Get parameter value and metadata.
inline const params::data& params::get_data(const string_id& id) const
{
  return _parameters[id];
}

// Set parameter value.
inline void params::set(const string_id& id, const type_variant& value)
{
  data& data{_parameters[id]};
  if (data.value != value) {
    data.value = value;
    ++data.version;
  }
}

inline float params::get_float(const string_id& id) const
{
  return std::get<float>(_parameters[id].value);
}

inline bool params::get_bool(const string_id& id) const
{
  return std::get<bool>(_parameters[id].value);
}
}  // namespace eely