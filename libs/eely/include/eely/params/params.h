#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <any>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <variant>

namespace eely {
// Use `std::variant` instead of `std::any`
// to avoid breaking SBO in `std::any` which will lead to dynamic allocatinos.
// E.g. parameters with three floats that can be fed as IK positions will probably break it.
using param_value = std::variant<std::monostate, int, float, bool>;

// Describes external parameters that are used as inputs to an animation graph.
// TODO: assign indices to params and use them instead of map lookups.
struct params final {
public:
  // Get parameter as a variant.
  const param_value& get(const string_id& id) const;

  // Get parameter value.
  template <typename T>
  [[nodiscard]] const T& get_value(const string_id& id) const;

  // Get modifiable parameter value.
  template <typename T>
  [[nodiscard]] T& get_value(const string_id& id);

private:
  // Mutable to intiailize default values in const getters.
  mutable std::unordered_map<string_id, param_value> _parameters;
};

namespace internal {
static constexpr gsl::index bits_param_value_type_index{5};
static constexpr gsl::index bits_param_value_int{16};

// Return `param_value` value read from a memory buffer.
template <>
param_value bit_reader_read(bit_reader& reader);

// Write `param_value` into a memory buffer.
void bit_writer_write(bit_writer& writer, const param_value& value);
}  // namespace internal

// Implementation

inline const param_value& params::get(const string_id& id) const
{
  return _parameters[id];
}

template <typename T>
const T& params::get_value(const string_id& id) const
{
  param_value& variant{_parameters[id]};

  if (std::get_if<std::monostate>(&variant) != nullptr) {
    variant = T{};
  }

  return std::get<T>(variant);
}

template <typename T>
T& params::get_value(const string_id& id)
{
  param_value& variant{_parameters[id]};

  if (std::get_if<std::monostate>(&variant) != nullptr) {
    variant = T{};
  }

  return std::get<T>(variant);
}

namespace internal {
template <>
inline param_value bit_reader_read(bit_reader& reader)
{
  param_value result;

  gsl::index type_index{bit_reader_read<gsl::index>(reader, bits_param_value_type_index)};
  switch (type_index) {
    case 0: {
      static_assert(std::is_same_v<std::variant_alternative_t<0, param_value>, std::monostate>);
    } break;

    case 1: {
      static_assert(std::is_same_v<std::variant_alternative_t<1, param_value>, int>);
      result = bit_reader_read<int>(reader, bits_param_value_int);
    } break;

    case 2: {
      static_assert(std::is_same_v<std::variant_alternative_t<2, param_value>, float>);
      result = bit_reader_read<float>(reader);
    } break;

    case 3: {
      static_assert(std::is_same_v<std::variant_alternative_t<3, param_value>, bool>);
      result = bit_reader_read<bool>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown param value type for reading");
    } break;
  }

  return result;
}
}  // namespace internal
}  // namespace eely