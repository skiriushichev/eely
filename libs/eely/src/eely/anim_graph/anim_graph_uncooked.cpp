#include "eely/anim_graph/anim_graph_uncooked.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/resource_uncooked.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
anim_graph_uncooked::anim_graph_uncooked(internal::bit_reader& reader) : resource_uncooked{reader}
{
  using namespace eely::internal;

  _skeleton_id = bit_reader_read<string_id>(reader);

  _nodes.resize(bit_reader_read<gsl::index>(reader, bits_anim_graph_nodes_size));
  for (gsl::index i{0}; i < std::ssize(_nodes); ++i) {
    _nodes[i] = bit_reader_read<anim_graph_node_uptr>(reader);
  }

  _root_node_id = bit_reader_read<std::optional<int>>(reader, bits_anim_graph_node_id);
}

anim_graph_uncooked::anim_graph_uncooked(const string_id& id) : resource_uncooked{id} {}

anim_graph_uncooked::anim_graph_uncooked(const anim_graph_uncooked& other)
    : resource_uncooked{other.get_id()}
{
  _skeleton_id = other._skeleton_id;

  _nodes.reserve(other._nodes.size());
  for (const auto& node : other._nodes) {
    _nodes.push_back(node->clone());
  }

  _root_node_id = other._root_node_id;
}

anim_graph_uncooked::anim_graph_uncooked(anim_graph_uncooked&& other) noexcept
    : resource_uncooked{other.get_id()}
{
  _skeleton_id = std::move(other._skeleton_id);
  _nodes = std::move(other._nodes);
  _root_node_id = other._root_node_id;
}

anim_graph_uncooked& anim_graph_uncooked::operator=(const anim_graph_uncooked& other)
{
  if (this != &other) {
    _skeleton_id = other._skeleton_id;

    _nodes.clear();
    _nodes.reserve(other._nodes.size());
    for (const auto& node : other._nodes) {
      _nodes.push_back(node->clone());
    }

    _root_node_id = other._root_node_id;
  }

  return *this;
}

anim_graph_uncooked& anim_graph_uncooked::operator=(anim_graph_uncooked&& other) noexcept
{
  if (this != &other) {
    _skeleton_id = std::move(other._skeleton_id);
    _nodes = std::move(other._nodes);
    _root_node_id = other._root_node_id;
  }

  return *this;
}

void anim_graph_uncooked::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  bit_writer_write(writer, _skeleton_id);

  bit_writer_write(writer, _nodes.size(), bits_anim_graph_nodes_size);
  for (gsl::index i{0}; i < std::ssize(_nodes); ++i) {
    const anim_graph_node_uptr& node{_nodes[i]};
    EXPECTS(node != nullptr);

    bit_writer_write(writer, *node);
  }

  bit_writer_write(writer, _root_node_id, bits_anim_graph_node_id);
}

void anim_graph_uncooked::collect_dependencies(
    std::unordered_set<string_id>& out_dependencies) const
{
  out_dependencies.insert(_skeleton_id);

  for (const anim_graph_node_uptr& node : _nodes) {
    node->collect_dependencies(out_dependencies);
  }
}

const string_id& anim_graph_uncooked::get_skeleton_id() const
{
  return _skeleton_id;
}

void anim_graph_uncooked::set_skeleton_id(string_id id)
{
  _skeleton_id = std::move(id);
}

const std::vector<anim_graph_node_uptr>& anim_graph_uncooked::get_nodes() const
{
  return _nodes;
}

std::optional<int> anim_graph_uncooked::get_root_node_id() const
{
  return _root_node_id;
}

void anim_graph_uncooked::set_root_node_id(std::optional<int> id)
{
  _root_node_id = id;
}

int anim_graph_uncooked::generate_node_id() const
{
  using namespace eely::internal;

  // This is not optimal, but simple enough and it's only for offline use.
  // Can be optimized if needed.

  std::unordered_set<int> existing_ids;
  for (const anim_graph_node_uptr& node : _nodes) {
    EXPECTS(node != nullptr);
    EXPECTS(!existing_ids.contains(node->get_id()));
    existing_ids.insert(node->get_id());
  }

  if (existing_ids.empty()) {
    return 0;
  }

  for (const int existing_id : existing_ids) {
    int new_id{existing_id + 1};
    if (new_id == internal::anim_graph_max_node_id) {
      new_id = 0;
    }

    if (!existing_ids.contains(new_id)) {
      return new_id;
    }
  }

  throw std::runtime_error{"Could not generate unique id for a new node. Is graph full?"};
}
}  // namespace eely