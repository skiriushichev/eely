#include "eely/params/params.h"

#include "eely/base/bit_writer.h"

#include <variant>

namespace eely::internal {
void bit_writer_write(bit_writer& writer, const param_value& value)
{
  bit_writer_write(writer, value.index(), bits_param_value_type_index);

  switch (value.index()) {
    case 0: {
      static_assert(std::is_same_v<std::variant_alternative_t<0, param_value>, std::monostate>);
    } break;

    case 1: {
      static_assert(std::is_same_v<std::variant_alternative_t<1, param_value>, int>);
      bit_writer_write(writer, std::get<int>(value), bits_param_value_int);
    } break;

    case 2: {
      static_assert(std::is_same_v<std::variant_alternative_t<2, param_value>, float>);
      bit_writer_write(writer, std::get<float>(value));
    } break;

    case 3: {
      static_assert(std::is_same_v<std::variant_alternative_t<3, param_value>, bool>);
      bit_writer_write(writer, std::get<bool>(value));
    } break;

    default: {
      throw std::runtime_error("Unknown param value type for writing");
    } break;
  }
}
}  // namespace eely::internal