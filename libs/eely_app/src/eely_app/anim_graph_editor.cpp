#include "eely_app/anim_graph_editor.h"

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
#include <eely/anim_graph/anim_graph_player_node_blend.h>
#include <eely/anim_graph/anim_graph_player_node_pose_base.h>
#include <eely/anim_graph/anim_graph_player_node_state_transition.h>
#include <eely/anim_graph/anim_graph_uncooked.h>
#include <eely/base/string_id.h>
#include <eely/clip/clip.h>
#include <eely/params/params.h>

#include <fmt/format.h>

#include <gsl/util>

#include <imgui.h>
#include <imgui_extra_math.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace eely {
// All ids in a node editor share number space,
// and zero is an invalid id.
// So we construct ids as following:
// - node data: node id + 1 [24 bits] | type [3 bits] | id within a type for this node [5 bits].
// - link idss: source pin id [32 bits] | target pin id [32 bits].
static_assert(sizeof(ax::NodeEditor::PinId) >= 4);
static_assert(sizeof(ax::NodeEditor::NodeId) >= 4);
static_assert(sizeof(ax::NodeEditor::LinkId) >= 8);

// List of entities existing within a node,
// used for ID generation.
enum class editor_node_entity_type { node, pin_input, pin_output };

// Constants for node's input pins on left and right sides.
// They have the same meaning, jsut different locations for better visuals.
// Since state machines introduce cycles in a graph,
// transitions use right node to avoid links being hidden under the nodes.
static constexpr gsl::index pin_input_index_node_left{0};
static constexpr gsl::index pin_input_index_node_right{1};

// Rendering and style constants

static const std::unordered_map<anim_graph_node_type, ImU32> node_type_to_accent_color{
    {anim_graph_node_type::and_logic, IM_COL32(250, 88, 182, 255)},
    {anim_graph_node_type::blend, IM_COL32(3, 201, 136, 255)},
    {anim_graph_node_type::clip, IM_COL32(33, 146, 255, 255)},
    {anim_graph_node_type::param_comparison, IM_COL32(39, 0, 130, 255)},
    {anim_graph_node_type::param, IM_COL32(250, 218, 157, 255)},
    {anim_graph_node_type::random, IM_COL32(220, 95, 0, 255)},
    {anim_graph_node_type::speed, IM_COL32(199, 128, 250, 255)},
    {anim_graph_node_type::state_condition, IM_COL32(238, 238, 238, 255)},
    {anim_graph_node_type::state_machine, IM_COL32(233, 69, 96, 255)},
    {anim_graph_node_type::state_transition, IM_COL32(20, 195, 142, 255)},
    {anim_graph_node_type::state, IM_COL32(173, 231, 146, 255)},
    {anim_graph_node_type::sum, IM_COL32(240, 238, 237, 255)}};

static constexpr float accent_mark_radius{5.0F};
static constexpr ImVec2 accent_mark_offset{10.0F, 10.0F};

static constexpr ImVec2 item_spacing{8.0F, 8.0F};
static constexpr ImVec4 node_padding{8.0F, 3.0F, 8.0F, 8.0F};

static constexpr float mid_arrow_size{15.0F};
static constexpr float mid_arrow_width{15.0F};

static constexpr ImVec4 weight_color_full{0.95F, 0.95F, 0.95F, 1.0F};
static constexpr ImVec4 weight_color_none{0.45F, 0.45F, 0.45F, 1.0F};
static constexpr ImVec4 weight_color_inactive{0.3F, 0.3F, 0.3F, 1.0F};

static constexpr float phase_bar_height{2.5F};
static constexpr ImVec2 phase_bar_indentation{20.0F, 4.0F};

static constexpr ImVec2 node_min_size_and{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_blend{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_clip{100.0F, 50.0F};
static constexpr ImVec2 node_min_size_param_comparison{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_param{100.0F, 50.0F};
static constexpr ImVec2 node_min_size_random{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_state_condition{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_state_machine{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_state_transition{250.0F, 50.0F};
static constexpr ImVec2 node_min_size_state{140.0F, 50.0F};
static constexpr ImVec2 node_min_size_speed{150.0F, 50.0F};
static constexpr ImVec2 node_min_size_sum{150.0F, 50.0F};

// Extract node id from editor node entity id (such as a pin).
static int get_node_id(const uint32_t editor_node_entity_id)
{
  const int node_id{static_cast<int>(editor_node_entity_id >> 8) - 1};
  return node_id;
}

// Generate id for an entity within a node.
static uint32_t get_editor_node_entity_id(const int node_id,
                                          const editor_node_entity_type entity_type,
                                          const gsl::index entity_id)
{
  using namespace eely::internal;

  static_assert(bits_anim_graph_node_id <= 24);
  EXPECTS(static_cast<int>(entity_type) < 8);
  EXPECTS(entity_id < 32);

  uint32_t id{0};

  id |= ((node_id + 1) << 8);
  id |= (static_cast<int>(entity_type) << 5);
  id |= entity_id;

  EXPECTS(id != 0);
  EXPECTS(get_node_id(id) == node_id);

  return id;
}

// Get node ID within the editor.
static ax::NodeEditor::NodeId get_editor_node_id(const int node_id)
{
  return get_editor_node_entity_id(node_id, editor_node_entity_type::node, 0);
}

// Get input pin ID within the editor.
static ax::NodeEditor::PinId get_editor_pin_input_id(const int node_id, const gsl::index pin_index)
{
  return get_editor_node_entity_id(node_id, editor_node_entity_type::pin_input, pin_index);
}

// Get output pin ID within the editor.
static ax::NodeEditor::PinId get_editor_pin_output_id(const int node_id, const gsl::index pin_index)
{
  return get_editor_node_entity_id(node_id, editor_node_entity_type::pin_output, pin_index);
}

// Get link ID within the editor.
static ax::NodeEditor::LinkId get_editor_link_id(const ax::NodeEditor::PinId editor_pin_id_from,
                                                 const ax::NodeEditor::PinId editor_pin_id_to)
{
  return ((editor_pin_id_from.Get() << 32) | editor_pin_id_to.Get());
}

// Set size a node will be rendered at a minimum,
// no matter the content.
static void set_node_min_size(const ImVec2 size)
{
  ImVec2 prev_pos{ImGui::GetCursorPos()};
  ImGui::Dummy(size);
  ImGui::SetCursorPos(prev_pos);
}

static void render_node_accent_mark(const anim_graph_node_base& node)
{
  const ax::NodeEditor::NodeId editor_node_id{get_editor_node_id(node.get_id())};

  const ImU32 color{node_type_to_accent_color.at(node.get_type())};
  const ImVec2 node_position{ax::NodeEditor::GetNodePosition(editor_node_id)};

  ImDrawList* draw_list{ax::NodeEditor::GetNodeBackgroundDrawList(editor_node_id)};
  draw_list->AddCircleFilled(node_position + accent_mark_offset, accent_mark_radius, color);
}

anim_graph_editor::deferred_imgui_render::deferred_imgui_render(
    deferred_imgui_render&& other) noexcept
    : _action_cursor_pos{other._action_cursor_pos}, _action{other._action}
{
  other._action = nullptr;
}

anim_graph_editor::deferred_imgui_render::~deferred_imgui_render()
{
  if (_action) {
    const ImVec2 prev_cursor_pos{ImGui::GetCursorPos()};
    ImGui::SetCursorScreenPos(_action_cursor_pos);

    _action();

    ImGui::SetCursorPos(prev_cursor_pos);
  }
}

void anim_graph_editor::deferred_imgui_render::reserve_space(const ImVec2 reserved_space)
{
  _action_cursor_pos = ImGui::GetCursorPos();
  ImGui::Dummy(reserved_space);
}

void anim_graph_editor::deferred_imgui_render::set_action(std::function<void(void)> action)
{
  _action = std::move(action);
}

anim_graph_editor::anim_graph_editor(const anim_graph& anim_graph, const anim_graph_player* player)
    : _editable{false},  // We're given already cooked resource, can't be edited
      _graph_id{anim_graph.get_id()},
      _nodes{anim_graph.get_nodes()},
      _player{player},
      _context{create_editor()}
{
}
anim_graph_editor::anim_graph_editor(const anim_graph_uncooked& anim_graph,
                                     const anim_graph_player* player)
    : _editable{true},
      _graph_id{anim_graph.get_id()},
      _nodes{anim_graph.get_nodes()},
      _player{player},
      _context{create_editor()}
{
}

anim_graph_editor::~anim_graph_editor()
{
  ax::NodeEditor::DestroyEditor(_context);
}

void anim_graph_editor::render()
{
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, item_spacing);

  ax::NodeEditor::SetCurrentEditor(_context);

  ax::NodeEditor::Begin(_graph_id.c_str());

  // Render nodes

  ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_NodePadding, node_padding);
  for (const anim_graph_node_uptr& node : _nodes) {
    EXPECTS(node != nullptr);
    render_node(*node.get());
  }
  ax::NodeEditor::PopStyleVar();

  // Render links
  // These must be rendered after all the nodes are ready,
  // so we put them in `_pending_links` instead of drawing them in-place.

  for (const link& l : _pending_links) {
    EXPECTS(l.pin_id_from != l.pin_id_to);
    const ax::NodeEditor::LinkId id{get_editor_link_id(l.pin_id_from, l.pin_id_to)};

    const int node_id_from{get_node_id(l.pin_id_from.Get())};
    const anim_graph_node_base* node_from{get_node(node_id_from)};
    EXPECTS(node_from != nullptr);

    const int node_id_to{get_node_id(l.pin_id_to.Get())};
    const anim_graph_node_base* node_to{get_node(node_id_to)};
    EXPECTS(node_to != nullptr);

    if (l.mid_arrow) {
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkMidArrowSize, mid_arrow_size);
      ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkMidArrowWidth, mid_arrow_width);
    }

    ax::NodeEditor::Link(id, l.pin_id_from, l.pin_id_to,
                         get_link_weight_color(*node_from, *node_to));

    if (l.mid_arrow) {
      ax::NodeEditor::PopStyleVar(2);
    }
  }
  _pending_links.clear();

  ax::NodeEditor::End();

  if (_first_render) {
    _first_render = false;

    // Zoom in/out on first render for the whole graph to fit.
    // BTW this breaks if drawn in a window with a scroll.
    ax::NodeEditor::NavigateToContent(0.0F);
  }

  ImGui::PopStyleVar(1);
}

ax::NodeEditor::EditorContext* anim_graph_editor::create_editor()
{
  ax::NodeEditor::Config config;
  config.SettingsFile = nullptr;

  return ax::NodeEditor::CreateEditor(&config);
}

const anim_graph_node_base* anim_graph_editor::get_node(int id) const
{
  for (const anim_graph_node_uptr& node : _nodes) {
    if (node->get_id() == id) {
      return node.get();
    }
  }

  return nullptr;
}

const internal::anim_graph_player_node_base* anim_graph_editor::get_player_node(
    const anim_graph_node_base& node) const
{
  if (_player == nullptr) {
    return nullptr;
  }

  return _player->get_player_node(node.get_id());
}

float anim_graph_editor::get_player_node_weight(
    const internal::anim_graph_player_node_base& player_node) const
{
  using namespace internal;

  EXPECTS(_player != nullptr);

  // Weight only makes sense when a node is used by another blend or state transition node.
  // So we need to find parent and check if this node is used by it,
  // and if it does - get the weight.

  // TODO: retrieve weight from state transition

  for (const anim_graph_player_node_uptr& other_node : _player->get_nodes()) {
    const anim_graph_node_type type{other_node->get_type()};

    switch (type) {
      case anim_graph_node_type::blend: {
        const auto& node_blend{
            polymorphic_downcast<const anim_graph_player_node_blend*>(other_node.get())};

        if (node_blend->get_current_source_node() == &player_node) {
          return 1.0F - node_blend->get_current_destination_node_weight();
        }

        if (node_blend->get_current_destination_node() == &player_node) {
          return node_blend->get_current_destination_node_weight();
        }
      } break;

      default: {
      } break;
    }
  }

  // This node is not used with weight, so it's either 0 or 1,
  // depending on if it's active on current play or not.
  return _player->is_player_node_active(player_node) ? 1.0F : 0.0F;
}

std::vector<const anim_graph_node_state*> anim_graph_editor::get_transition_sources(
    const anim_graph_node_state_transition& node_transition)
{
  using namespace internal;

  std::vector<const anim_graph_node_state*> result;

  for (const anim_graph_node_uptr& node : _nodes) {
    if (node->get_type() != anim_graph_node_type::state) {
      continue;
    }

    const anim_graph_node_state* node_state{
        polymorphic_downcast<const anim_graph_node_state*>(node.get())};

    for (const int transition_id : node_state->get_out_transition_nodes()) {
      if (transition_id == node_transition.get_id()) {
        result.push_back(node_state);
        break;
      }
    }
  }

  return result;
}

ImVec4 anim_graph_editor::get_node_weight_color(const anim_graph_node_base& node) const
{
  using namespace internal;

  const anim_graph_player_node_base* player_node{get_player_node(node)};
  if (player_node == nullptr) {
    // Use default style if there's no runtime player
    return weight_color_full;
  }

  ImVec4 color{weight_color_inactive};
  if (_player->is_player_node_active(*player_node)) {
    const float weight{get_player_node_weight(*player_node)};
    color = ImLerp(weight_color_none, weight_color_full, weight);
  }

  return color;
}

ImVec4 anim_graph_editor::get_link_weight_color(const anim_graph_node_base& node_from,
                                                const anim_graph_node_base& node_to) const
{
  using namespace internal;

  const anim_graph_player_node_base* player_node_from{get_player_node(node_from)};
  const anim_graph_player_node_base* player_node_to{get_player_node(node_to)};
  if (player_node_to == nullptr || player_node_from == nullptr) {
    // Use default style if there's no runtime player
    return weight_color_full;
  }

  // If destination node is a state, only show its weight if its parent is active.
  // Because state nodes can have multiple incoming connections,
  // specifically connection from a state machine and from transitions.
  // For other nodes use default rules - if destination node is active, color link with its weight.

  bool link_is_active{false};

  if (player_node_to->get_type() == anim_graph_node_type::state) {
    if (_player->is_player_node_active(*player_node_from)) {
      link_is_active = true;
    }
  }
  else if (_player->is_player_node_active(*player_node_to)) {
    link_is_active = true;
  }

  ImVec4 color{weight_color_inactive};
  if (link_is_active) {
    const float weight{get_player_node_weight(*player_node_to)};
    color = ImLerp(weight_color_none, weight_color_full, weight);
  }

  return color;
}

void anim_graph_editor::render_param_value(const param_value& value) const
{
  static_assert(std::is_same_v<std::variant_alternative_t<0, param_value>, std::monostate>);
  static_assert(std::is_same_v<std::variant_alternative_t<1, param_value>, int>);
  static_assert(std::is_same_v<std::variant_alternative_t<2, param_value>, float>);
  static_assert(std::is_same_v<std::variant_alternative_t<3, param_value>, bool>);

  static const std::array<std::string, 4> types{"none", "int", "float", "bool"};

  ImGui::BeginHorizontal("param_value");
  {
    ImGui::TextUnformatted("type:");
    ImGui::BeginDisabled(!_editable);
    ImGui::Button(types.at(value.index()).c_str());
    ImGui::EndDisabled();

    ImGui::Spacing();

    ImGui::TextUnformatted("value:");
    ImGui::BeginDisabled(!_editable);
    switch (value.index()) {
      case 0: {
      } break;

      case 1: {
        ImGui::TextUnformatted(fmt::format("{}", std::get<int>(value)).c_str());
      } break;

      case 2: {
        ImGui::TextUnformatted(fmt::format("{:.2f}", std::get<float>(value)).c_str());
      } break;

      case 3: {
        ImGui::TextUnformatted(fmt::format("{}", std::get<bool>(value)).c_str());
      } break;

      default: {
        throw std::runtime_error("Unknown param value type for rendering");
      } break;
    }
    ImGui::EndDisabled();
  }
  ImGui::EndHorizontal();
}

// Render default node header: input pins and header with springs.
void anim_graph_editor::render_node_header_default(const anim_graph_node_base& node,
                                                   const std::string& title)
{
  using namespace internal;

  ImGui::BeginHorizontal("header");
  {
    render_pin(node, ax::NodeEditor::PinKind::Input, pin_input_index_node_left, " ",
               pin_location::left);

    ImGui::Spring();

    ImGui::TextUnformatted(title.c_str());

    ImGui::Spring();

    render_pin(node, ax::NodeEditor::PinKind::Input, pin_input_index_node_right, " ",
               pin_location::right);
  }
  ImGui::EndHorizontal();

  ImGui::Spacing();
}

// Render a pin and return its editor ID.
ax::NodeEditor::PinId anim_graph_editor::render_pin(const anim_graph_node_base& node,
                                                    const ax::NodeEditor::PinKind pin_kind,
                                                    const gsl::index pin_index,
                                                    const std::string& label,
                                                    const pin_location location)
{
  // Node editor breaks if we have an invisible pin
  EXPECTS(!label.empty());

  const int node_id{node.get_id()};

  ax::NodeEditor::PinId editor_pin_id;

  if (pin_kind == ax::NodeEditor::PinKind::Input) {
    editor_pin_id = get_editor_pin_input_id(node_id, pin_index);
  }
  else {
    editor_pin_id = get_editor_pin_output_id(node_id, pin_index);
  }

  ImVec2 pin_pivot_alignment;
  ImVec2 pin_source_direction;
  ImVec2 pin_target_direction;
  if (location == pin_location::left) {
    pin_pivot_alignment = ImVec2{0.0F, 0.5F};
    pin_source_direction = ImVec2{-1.0F, 0.0F};
    pin_target_direction = ImVec2{-1.0F, 0.0F};
  }
  else {
    pin_pivot_alignment = ImVec2{1.0F, 0.5F};
    pin_source_direction = ImVec2{1.0F, 0.0F};
    pin_target_direction = ImVec2{1.0F, 0.0F};
  }

  ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, pin_source_direction);
  ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, pin_target_direction);

  ax::NodeEditor::BeginPin(editor_pin_id, pin_kind);
  ax::NodeEditor::PinPivotAlignment(pin_pivot_alignment);
  ImGui::TextUnformatted(label.c_str());
  ax::NodeEditor::EndPin();

  ax::NodeEditor::PopStyleVar(2);

  return editor_pin_id;
}

void anim_graph_editor::render_output_pin_and_links(const anim_graph_node_base& node,
                                                    gsl::index pin_index,
                                                    pin_location pin_location,
                                                    const std::string& label,
                                                    const std::vector<int>& child_nodes,
                                                    bool mid_arrow)
{
  const ax::NodeEditor::PinId pin_output_id{
      render_pin(node, ax::NodeEditor::PinKind::Output, pin_index, label, pin_location)};

  for (gsl::index i{0}; i < std::ssize(child_nodes); ++i) {
    const gsl::index child_node_pin_index{pin_location == anim_graph_editor::pin_location::left
                                              ? pin_input_index_node_right
                                              : pin_input_index_node_left};

    _pending_links.push_back(
        {.pin_id_from = pin_output_id,
         .pin_id_to = get_editor_pin_input_id(child_nodes[i], child_node_pin_index),
         .mid_arrow = mid_arrow});
  }
}

void anim_graph_editor::render_output_pin_and_link(const anim_graph_node_base& node,
                                                   const gsl::index pin_index,
                                                   pin_location pin_location,
                                                   const std::string& label,
                                                   std::optional<int> child_node_id,
                                                   bool mid_arrow)
{
  const ax::NodeEditor::PinId pin_output_id{
      render_pin(node, ax::NodeEditor::PinKind::Output, pin_index, label, pin_location)};

  if (child_node_id.has_value()) {
    const gsl::index child_node_pin_index{pin_location == anim_graph_editor::pin_location::left
                                              ? pin_input_index_node_right
                                              : pin_input_index_node_left};
    _pending_links.push_back(
        {.pin_id_from = pin_output_id,
         .pin_id_to = get_editor_pin_input_id(child_node_id.value(), child_node_pin_index),
         .mid_arrow = mid_arrow});
  }
}

anim_graph_editor::deferred_imgui_render anim_graph_editor::create_phase_bar_deferred_renderer(
    const anim_graph_node_base& node)
{
  using namespace internal;

  anim_graph_editor::deferred_imgui_render renderer;

  const anim_graph_player_node_base* player_node{get_player_node(node)};

  if (const auto* player_node_pose{
          dynamic_cast<const anim_graph_player_node_pose_base*>(player_node)}) {
    // Reserve space even if this node is not active.
    // So that node's size doesn't abruptly changes whenever it switches.
    renderer.reserve_space(ImVec2{phase_bar_height + phase_bar_indentation.y * 2.0F, 0});

    const ax::NodeEditor::NodeId editor_node_id{get_editor_node_id(node.get_id())};

    if (_player->is_player_node_active(*player_node_pose)) {
      // Set rendering action since the node is active
      renderer.set_action([player_node_pose, editor_node_id]() {
        const ImVec2 node_position{ax::NodeEditor::GetNodePosition(editor_node_id)};
        const ImVec2 node_size{ax::NodeEditor::GetNodeSize(editor_node_id)};

        const float phase_bar_width{player_node_pose->get_phase() *
                                    (node_size.x - 2.0F * phase_bar_indentation.x)};

        const ImVec2 phase_bar_position_top_left{node_position + phase_bar_indentation};
        const ImVec2 phase_bar_position_bottom_right{phase_bar_position_top_left +
                                                     ImVec2{phase_bar_width, phase_bar_height}};

        ImDrawList* draw_list{ax::NodeEditor::GetNodeBackgroundDrawList(editor_node_id)};

        draw_list->AddRectFilled(phase_bar_position_top_left, phase_bar_position_bottom_right,
                                 IM_COL32_WHITE, ax::NodeEditor::GetStyle().NodeRounding);
      });
    }
  }

  return renderer;
}

void anim_graph_editor::render_node(const anim_graph_node_base& node)
{
  using namespace eely::internal;

  const ax::NodeEditor::NodeId editor_node_id{get_editor_node_id(node.get_id())};

  const float3& editor_position{node.get_editor_position()};
  if (_first_render) {
    // Position node on first render the way it was saved
    ax::NodeEditor::SetNodePosition(editor_node_id, ImVec2{editor_position.x, editor_position.y});
    ax::NodeEditor::SetNodeZPosition(editor_node_id, editor_position.z);
  }

  ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder,
                                 get_node_weight_color(node));

  // We need to `ImGui::PushID` as well,
  // because nested scope ids can be repeated between nodes.
  ax::NodeEditor::BeginNode(editor_node_id);
  ImGui::PushID(&node);

  // Draw phase from a runtime node, if available
  deferred_imgui_render player_node_phase_renderer{create_phase_bar_deferred_renderer(node)};

  const anim_graph_node_type type{node.get_type()};
  switch (type) {
    case anim_graph_node_type::and_logic: {
      render_node_and(*polymorphic_downcast<const anim_graph_node_and*>(&node));
    } break;

    case anim_graph_node_type::blend: {
      render_node_blend(*polymorphic_downcast<const anim_graph_node_blend*>(&node));
    } break;

    case anim_graph_node_type::clip: {
      render_node_clip(*polymorphic_downcast<const anim_graph_node_clip*>(&node));
    } break;

    case anim_graph_node_type::param_comparison: {
      render_node_param_comparison(
          *polymorphic_downcast<const anim_graph_node_param_comparison*>(&node));
    } break;

    case anim_graph_node_type::param: {
      render_node_param(*polymorphic_downcast<const anim_graph_node_param*>(&node));
    } break;

    case anim_graph_node_type::random: {
      render_node_random(*polymorphic_downcast<const anim_graph_node_random*>(&node));
    } break;

    case anim_graph_node_type::speed: {
      render_node_speed(*polymorphic_downcast<const anim_graph_node_speed*>(&node));
    } break;

    case anim_graph_node_type::state_condition: {
      render_node_state_condition(
          *polymorphic_downcast<const anim_graph_node_state_condition*>(&node));
    } break;

    case anim_graph_node_type::state_machine: {
      render_node_state_machine(*polymorphic_downcast<const anim_graph_node_state_machine*>(&node));
    } break;

    case anim_graph_node_type::state_transition: {
      render_node_state_transition(
          *polymorphic_downcast<const anim_graph_node_state_transition*>(&node));
    } break;

    case anim_graph_node_type::state: {
      render_node_state(*polymorphic_downcast<const anim_graph_node_state*>(&node));
    } break;

    case anim_graph_node_type::sum: {
      render_node_sum(*polymorphic_downcast<const anim_graph_node_sum*>(&node));
    } break;

    default: {
    } break;
  }

  ImGui::PopID();
  ax::NodeEditor::EndNode();

  render_node_accent_mark(node);

  ax::NodeEditor::PopStyleColor(1);
}

void anim_graph_editor::render_node_and(const anim_graph_node_and& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_and);

    render_node_header_default(node, "AND");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();
      render_output_pin_and_links(node, 0, pin_location::right, "conditions",
                                  node.get_children_nodes());
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_blend(const anim_graph_node_blend& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_blend);

    render_node_header_default(node, "BLEND");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();

      ImGui::BeginVertical("pins_output");
      {
        // Factor node
        render_output_pin_and_link(node, 0, pin_location::right, "factor",
                                   node.get_factor_node_id());

        ImGui::Spacing();

        // Pose nodes
        const std::vector<anim_graph_node_blend::pose_node_data>& pose_nodes{node.get_pose_nodes()};
        for (gsl::index i{0}; i < std::ssize(pose_nodes); ++i) {
          const anim_graph_node_blend::pose_node_data& pose_node_data{pose_nodes[i]};
          render_output_pin_and_link(node, i + 1, pin_location::right,
                                     fmt::format("{:.2f}", pose_node_data.factor),
                                     pose_node_data.id);
        }
      }
      ImGui::EndVertical();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_clip(const anim_graph_node_clip& node) const
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_clip);

    render_node_header_default(node, "CLIP");

    ImGui::BeginHorizontal("body");
    {
      ImGui::TextUnformatted("id:");
      ImGui::BeginDisabled(!_editable);
      ImGui::Button(node.get_clip_id().c_str());
      ImGui::EndDisabled();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_param_comparison(
    const anim_graph_node_param_comparison& node) const
{
  using op = anim_graph_node_param_comparison::op;

  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_param_comparison);

    render_node_header_default(node, "PARAMETER COMPARISON");

    ImGui::BeginHorizontal("param_id");
    {
      ImGui::TextUnformatted("id:");
      ImGui::BeginDisabled(!_editable);
      ImGui::Button(node.get_param_id().c_str());
      ImGui::EndDisabled();
    }
    ImGui::EndHorizontal();

    ImGui::Spacing();

    ImGui::BeginHorizontal("operation");
    {
      ImGui::TextUnformatted("operation:");
      ImGui::BeginDisabled(!_editable);
      ImGui::Button(node.get_op() == op::equal ? "equal" : "not equal");
      ImGui::EndDisabled();
    }
    ImGui::EndHorizontal();

    ImGui::Spacing();

    render_param_value(node.get_value());
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_param(const anim_graph_node_param& node) const
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_param);

    render_node_header_default(node, "PARAMETER");

    ImGui::BeginHorizontal("body");
    {
      ImGui::TextUnformatted("id:");
      ImGui::BeginDisabled(!_editable);
      ImGui::Button(node.get_param_id().c_str());
      ImGui::EndDisabled();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_random(const anim_graph_node_random& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_random);

    render_node_header_default(node, "RANDOM");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();
      render_output_pin_and_links(node, 0, pin_location::right, "choices",
                                  node.get_children_nodes());
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_state_condition(
    const anim_graph_node_state_condition& node) const
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_state_condition);

    render_node_header_default(node, "STATE CONDITION");

    ImGui::BeginHorizontal("phase");
    {
      ImGui::BeginDisabled(!_editable);
      bool phase_is_enabled{node.get_phase().has_value()};
      ImGui::Checkbox("phase", &phase_is_enabled);
      ImGui::EndDisabled();

      ImGui::Spacing();

      ImGui::TextUnformatted("value:");

      ImGui::BeginDisabled(!_editable);
      ImGui::TextUnformatted(fmt::format("{:.2f}", node.get_phase().value_or(0.0F)).c_str());
      ImGui::EndDisabled();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_state_machine(const anim_graph_node_state_machine& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_state_machine);

    render_node_header_default(node, "STATE_MACHINE");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();
      render_output_pin_and_links(node, 0, pin_location::right, "states", node.get_state_nodes());
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_state_transition(const anim_graph_node_state_transition& node)
{
  using namespace internal;

  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_state_transition);

    std::vector<const anim_graph_node_state*> source_states{get_transition_sources(node)};
    const gsl::index source_states_count{std::ssize(source_states)};

    std::ostringstream stream;
    stream << "STATE TRANSITION [";
    for (gsl::index i{0}; i < source_states_count; ++i) {
      stream << source_states[i]->get_name();
      if (i != source_states_count - 1) {
        stream << ", ";
      }
    }

    std::optional<int> destination_state_node_id{node.get_destination_state_node()};
    if (destination_state_node_id.has_value()) {
      const anim_graph_node_base* destination_node{get_node(destination_state_node_id.value())};
      const auto* destination_state_node{
          polymorphic_downcast<const anim_graph_node_state*>(destination_node)};
      EXPECTS(destination_state_node != nullptr);

      stream << " -> " << destination_state_node->get_name();
    }

    stream << "]";

    render_node_header_default(node, stream.str());

    ImGui::BeginHorizontal("pins_output");
    {
      render_output_pin_and_link(node, 0, pin_location::left, "destination",
                                 node.get_destination_state_node(), true);

      ImGui::Spring();

      render_output_pin_and_link(node, 1, pin_location::right, "condition",
                                 node.get_condition_node());
    }
    ImGui::EndHorizontal();

    ImGui::Spacing();

    ImGui::BeginVertical("settings");
    {
      ImGui::BeginHorizontal("transition_type");
      {
        ImGui::TextUnformatted("type:");
        ImGui::BeginDisabled(!_editable);
        ImGui::Button("frozen fade");
        ImGui::EndDisabled();
      }
      ImGui::EndHorizontal();

      ImGui::BeginHorizontal("duration");
      {
        ImGui::TextUnformatted("duration (s):");

        // TODO: edit
        ImGui::BeginDisabled(!_editable);
        ImGui::TextUnformatted(fmt::format("{:.2f}", node.get_duration_s()).c_str());
        ImGui::EndDisabled();
      }
      ImGui::EndHorizontal();
    }
    ImGui::EndVertical();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_state(const anim_graph_node_state& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_state);

    const string_id& name{node.get_name()};
    render_node_header_default(node, fmt::format("STATE [{}]", name.empty() ? "<unnamed>" : name));

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();

      ImGui::BeginVertical("pins_output");
      {
        render_output_pin_and_link(node, 0, pin_location::right, "pose", node.get_pose_node());
        render_output_pin_and_links(node, 1, pin_location::right, "transitions",
                                    node.get_out_transition_nodes(), true);
      }
      ImGui::EndVertical();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_speed(const anim_graph_node_speed& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_speed);

    render_node_header_default(node, "PLAYBACK SPEED");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();

      ImGui::BeginVertical("pins_output");
      {
        render_output_pin_and_link(node, 0, pin_location::right, "speed",
                                   node.get_speed_provider_node());
        render_output_pin_and_link(node, 1, pin_location::right, "child", node.get_child_node());
      }
      ImGui::EndVertical();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}

void anim_graph_editor::render_node_sum(const anim_graph_node_sum& node)
{
  ImGui::BeginVertical("root");
  {
    set_node_min_size(node_min_size_sum);

    render_node_header_default(node, "SUM");

    ImGui::BeginHorizontal("body");
    {
      ImGui::Spring();

      ImGui::BeginVertical("pins_output");
      {
        render_output_pin_and_link(node, 0, pin_location::right, "first", node.get_first_node_id());
        render_output_pin_and_link(node, 1, pin_location::right, "second",
                                   node.get_second_node_id());
      }
      ImGui::EndVertical();
    }
    ImGui::EndHorizontal();
  }
  ImGui::EndVertical();
}
}  // namespace eely