#pragma once

#include "eely/base/string_id.h"

#include <cstdint>
#include <unordered_map>

namespace eely {
// Describes external parameters that are used as inputs to a blend tree or a state machine.
// TODO: assign indexes to params and use them instead of map lookups.
struct params final {
public:
  // Get float parameter.
  [[nodiscard]] float get_float(const string_id& id) const;

  // Set float parameter.
  void set_float(const string_id& id, float value);

  // Get current version of these parameters.
  // Version is incremented every time a parameter changes.
  [[nodiscard]] uint32_t get_version() const;

private:
  // Mutable to intiailize default values in const getters.
  mutable std::unordered_map<string_id, float> _floats;

  uint32_t _version{0};
};

// Implementation

inline float params::get_float(const string_id& id) const
{
  return _floats[id];
}

inline void params::set_float(const string_id& id, float value)
{
  _floats[id] = value;
  ++_version;
}

inline uint32_t params::get_version() const
{
  return _version;
}
}  // namespace eely