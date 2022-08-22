#include "eely/btree/btree_node_blend.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <vector>

namespace eely {
btree_node_blend::btree_node_blend(bit_reader& reader) : btree_node_base(reader)
{
  using namespace eely::internal;

  _param_id = string_id_deserialize(reader);

  const gsl::index param_values_size{reader.read(bits_btree_node_index)};
  _children_param_values.resize(param_values_size);

  for (gsl::index i{0}; i < param_values_size; ++i) {
    _children_param_values[i] = bit_cast<float>(reader.read(32));
  }
}

void btree_node_blend::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  btree_node_base::serialize(writer);

  string_id_serialize(_param_id, writer);

  const gsl::index param_values_size{std::ssize(_children_param_values)};
  writer.write(
      {.value = gsl::narrow<uint32_t>(param_values_size), .size_bits = bits_btree_node_index});

  for (gsl::index i{0}; i < param_values_size; ++i) {
    writer.write({.value = bit_cast<uint32_t>(_children_param_values[i]), .size_bits = 32});
  };
}

std::unique_ptr<btree_node_base> btree_node_blend::clone() const
{
  return std::make_unique<btree_node_blend>(*this);
}

const string_id& btree_node_blend::get_param_id() const
{
  return _param_id;
}

void btree_node_blend::set_param_id(string_id param_id)
{
  _param_id = std::move(param_id);
}

const std::vector<float>& btree_node_blend::get_children_param_values() const
{
  return _children_param_values;
}

std::vector<float>& btree_node_blend::get_children_param_values()
{
  return _children_param_values;
}
}  // namespace eely