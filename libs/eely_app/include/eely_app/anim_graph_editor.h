#pragma once

#include <eely/anim_graph/anim_graph.h>
#include <eely/anim_graph/anim_graph_node_and.h>
#include <eely/anim_graph/anim_graph_node_base.h>
#include <eely/anim_graph/anim_graph_node_blend.h>
#include <eely/anim_graph/anim_graph_node_clip.h>
#include <eely/anim_graph/anim_graph_node_param.h>
#include <eely/anim_graph/anim_graph_node_param_comparison.h>
#include <eely/anim_graph/anim_graph_node_random.h>
#include <eely/anim_graph/anim_graph_node_speed.h>
#include <eely/anim_graph/anim_graph_node_state.h>
#include <eely/anim_graph/anim_graph_node_state_condition.h>
#include <eely/anim_graph/anim_graph_node_state_machine.h>
#include <eely/anim_graph/anim_graph_node_state_transition.h>
#include <eely/anim_graph/anim_graph_node_sum.h>
#include <eely/anim_graph/anim_graph_player.h>
#include <eely/anim_graph/anim_graph_player_node_base.h>
#include <eely/anim_graph/anim_graph_uncooked.h>
#include <eely/base/string_id.h>
#include <eely/params/params.h>

#include <imgui_node_editor.h>

#include <functional>
#include <vector>

namespace eely {
// Renders and edits (TODO) animation graph resource.
// If a graph is being played, also shows current graph's state,
// e.g. which nodes are active and their weights etc.
class anim_graph_editor final {
public:
  explicit anim_graph_editor(const anim_graph& anim_graph, const anim_graph_player* player);
  explicit anim_graph_editor(const anim_graph_uncooked& anim_graph,
                             const anim_graph_player* player);
  anim_graph_editor(const anim_graph_editor&) = delete;
  anim_graph_editor(anim_graph_editor&&) = delete;

  ~anim_graph_editor();

  anim_graph_editor& operator=(const anim_graph_editor&) = delete;
  anim_graph_editor& operator=(anim_graph_editor&&) = delete;

  void render();

private:
  // Class that reserves space for drawing something that can only be drawn later.
  // Used to draw phase bars etc., since their width is only known after the rest of the node
  // is drawn, but we need to put it on top.
  // This provides a neat RAII way to do that.
  class deferred_imgui_render final {
  public:
    deferred_imgui_render() = default;

    deferred_imgui_render(const deferred_imgui_render&) = delete;
    deferred_imgui_render(deferred_imgui_render&& other) noexcept;

    deferred_imgui_render& operator=(const deferred_imgui_render&) = delete;
    deferred_imgui_render& operator=(deferred_imgui_render&&) = delete;

    ~deferred_imgui_render();

    void reserve_space(ImVec2 reserved_space);
    void set_action(std::function<void(void)> action);

  private:
    ImVec2 _action_cursor_pos{};
    std::function<void(void)> _action;
  };

  // Describes pending link.
  struct link final {
    ax::NodeEditor::PinId pin_id_from;
    ax::NodeEditor::PinId pin_id_to;
    bool mid_arrow{false};
  };

  // Describes location of a pin in a node:
  // left or right side.
  enum class pin_location { left, right };

  static ax::NodeEditor::EditorContext* create_editor();

  // Data getters

  [[nodiscard]] const anim_graph_node_base* get_node(int id) const;
  [[nodiscard]] const internal::anim_graph_player_node_base* get_player_node(
      const anim_graph_node_base& node) const;
  [[nodiscard]] float get_player_node_weight(
      const internal::anim_graph_player_node_base& player_node) const;
  [[nodiscard]] std::vector<const anim_graph_node_state*> get_transition_sources(
      const anim_graph_node_state_transition& node_transition);

  // Rendering

  [[nodiscard]] ImVec4 get_node_weight_color(const anim_graph_node_base& node) const;
  [[nodiscard]] ImVec4 get_link_weight_color(const anim_graph_node_base& node_from,
                                             const anim_graph_node_base& node_to) const;

  void render_param_value(const param_value& value) const;

  static void render_node_header_default(const anim_graph_node_base& node,
                                         const std::string& title);
  static ax::NodeEditor::PinId render_pin(const anim_graph_node_base& node,
                                          ax::NodeEditor::PinKind pin_kind,
                                          gsl::index pin_index,
                                          const std::string& label,
                                          pin_location location);
  void render_output_pin_and_links(const anim_graph_node_base& node,
                                   gsl::index pin_index,
                                   pin_location pin_location,
                                   const std::string& label,
                                   const std::vector<int>& child_nodes,
                                   bool mid_arrow = false);
  void render_output_pin_and_link(const anim_graph_node_base& node,
                                  gsl::index pin_index,
                                  pin_location pin_location,
                                  const std::string& label,
                                  std::optional<int> child_node_id,
                                  bool mid_arrow = false);
  deferred_imgui_render create_phase_bar_deferred_renderer(const anim_graph_node_base& node);

  void render_node(const anim_graph_node_base& node);
  void render_node_and(const anim_graph_node_and& node);
  void render_node_blend(const anim_graph_node_blend& node);
  void render_node_clip(const anim_graph_node_clip& node) const;
  void render_node_param_comparison(const anim_graph_node_param_comparison& node) const;
  void render_node_param(const anim_graph_node_param& node) const;
  void render_node_random(const anim_graph_node_random& node);
  void render_node_state_condition(const anim_graph_node_state_condition& node) const;
  void render_node_state_machine(const anim_graph_node_state_machine& node);
  void render_node_state_transition(const anim_graph_node_state_transition& node);
  void render_node_state(const anim_graph_node_state& node);
  void render_node_speed(const anim_graph_node_speed& node);
  void render_node_sum(const anim_graph_node_sum& node);

  bool _editable;
  string_id _graph_id;
  const std::vector<anim_graph_node_uptr>& _nodes;
  const anim_graph_player* _player;

  ax::NodeEditor::EditorContext* _context;
  bool _first_render{true};
  std::vector<link> _pending_links;
};
}  // namespace eely