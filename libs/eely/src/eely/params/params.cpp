#include "eely/params/params.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <gsl/narrow>

#include <stdexcept>

namespace eely::internal {
constexpr gsl::index bits_param_type = 2;

void params_value_serialize(const params::type_variant& value, bit_writer& writer)
{
  writer.write({.value = gsl::narrow<uint32_t>(value.index()), .size_bits = bits_param_type});

  if (const auto* value_raw{std::get_if<float>(&value)}) {
    writer.write({.value = bit_cast<uint32_t>(*value_raw), .size_bits = 32});
  }
  else if (const auto* value_raw{std::get_if<bool>(&value)}) {
    writer.write({.value = (*value_raw) ? 1U : 0U, .size_bits = 1});
  }
}

// Deserialize parameter value previously written via `params_value_serialize`.
params::type_variant params_value_deserialize(bit_reader& reader)
{
  gsl::index index{reader.read(bits_param_type)};
  if (index == 0) {
    return params::type_variant{bit_cast<float>(reader.read(32))};
  }

  if (index == 1) {
    return params::type_variant{static_cast<bool>(reader.read(1))};
  }

  throw std::runtime_error{"Invalid variant index type for params value."};
}
}  // namespace eely::internal