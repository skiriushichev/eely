#pragma once

#include <SDL_scancode.h>

#include <array>
#include <optional>

namespace eely {
// Represents state of a digital input (button).
enum class input_digital {
  released,
  pressed,
  just_released,
  just_pressed,
};

// Represents state of an analog input (mouse, joystick).
struct input_analog final {
  float x{0.0F};
  float y{0.0F};
};

// Represents state of input devices.
class inputs final {
public:
  // Start updating input state.
  // Must be followed by a single call to `update_end`.
  // Input state cannot be read between `update_begin` and `update_end` calls.
  void update_begin();

  // Finish updating input state.
  // Must be preceded by a single call to `update_begin`.
  // Input state cannot be read between `update_begin` and `update_end` calls.
  void update_end();

  // Update keyboard button state.
  // Must be preceded by a single call to `update_begin`.
  void update_keyboard_button(SDL_Scancode scancode, bool pressed);

  // Update mouse button state.
  // Must be preceded by a single call to `update_begin`.
  void update_mouse_button(Uint8 button_index, bool pressed);

  // Update mouse cursor state.
  // Must be preceded by a single call to `update_begin`.
  void update_mouse_cursor(const input_analog& absolute,
                           const input_analog& relative);

  // Return state of a keyboard button.
  [[nodiscard]] input_digital get_keyboard_button(SDL_Scancode scancode) const;

  // return state of a mouse button.
  [[nodiscard]] input_digital get_mouse_button(Uint8 button_index) const;

  // Return mouse cursor's absolute position.
  [[nodiscard]] input_analog get_mouse_cursor_absolute() const;

  // Return mouse cursors' position relative to previous update.
  [[nodiscard]] input_analog get_mouse_cursor_relative() const;

private:
  static constexpr size_t keyboard_buttons_count{SDL_NUM_SCANCODES};
  static constexpr size_t mouse_buttons_count{5};

  std::array<input_digital, keyboard_buttons_count> _keyboard_buttons{};
  std::array<input_digital, mouse_buttons_count> _mouse_buttons{};
  input_analog _mouse_cursor;
  input_analog _mouse_cursor_delta;
};
}  // namespace eely