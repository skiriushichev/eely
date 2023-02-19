#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"

#include <any>
#include <memory>
#include <optional>
#include <vector>

namespace eely::internal {
// Base class for animation graph runtime nodes.
// These nodes produce poses or data for other nodes.
class anim_graph_player_node_base {
public:
  // Construct player node with specified type and ID.
  // ID must match cooked node counterpart.
  explicit anim_graph_player_node_base(anim_graph_node_type type, int id);

  virtual ~anim_graph_player_node_base() = default;

  // Compute node's result.
  // `std::any` is used assuming it is implemented with SBO
  // and we don't use types that don't fit into it.
  // TODO: can this be checked with an assert? What if it breaks?
  std::any compute(const anim_graph_player_context& context);

  // Collect all descendant nodes recursively.
  virtual void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& /*out_descendants*/) const {};

  // Return this node's type.
  [[nodiscard]] anim_graph_node_type get_type() const;

  // Return this node's id unique within a graph.
  [[nodiscard]] int get_id() const;

  // Return last graph's play counter during which this node was active.
  [[nodiscard]] std::optional<int> get_last_play_counter() const;

protected:
  virtual void compute_impl(const anim_graph_player_context& context, std::any& out_result);

  // Return `true` if this a first time a node is played,
  // i.e. either graph is played for the first time,
  // or after at least one graph update when this node was inactive.
  // Can be used to reset node's state if needed.
  [[nodiscard]] bool is_first_play(const anim_graph_player_context& context) const;

private:
  // Remember this node being played on current update.
  void register_play(const anim_graph_player_context& context) const;

  anim_graph_node_type _type;
  int _id;

  // These two are mutable for `if_first_play` to be a const API.
  // They are just cached values.
  mutable std::optional<int> _last_graph_play_counter;
  mutable bool _is_first_play{false};
};

// Shorter name for unique pointer to a player node.
using anim_graph_player_node_uptr = std::unique_ptr<anim_graph_player_node_base>;

// Implementation

inline anim_graph_node_type anim_graph_player_node_base::get_type() const
{
  return _type;
}

inline int anim_graph_player_node_base::get_id() const
{
  return _id;
}

inline std::optional<int> anim_graph_player_node_base::get_last_play_counter() const
{
  return _last_graph_play_counter;
}

inline bool anim_graph_player_node_base::is_first_play(
    const anim_graph_player_context& context) const
{
  register_play(context);
  return _is_first_play;
}

inline void anim_graph_player_node_base::register_play(
    const anim_graph_player_context& context) const
{
  if (_last_graph_play_counter != context.play_counter) {
    _is_first_play = (!_last_graph_play_counter.has_value() ||
                      (context.play_counter != _last_graph_play_counter.value() + 1));
    _last_graph_play_counter = context.play_counter;
  }
}
}  // namespace eely::internal