#include "eely_app/inputs.h"

#include <gsl/util>

#include <SDL_scancode.h>

namespace eely {
static input_digital input_digital_promote(const input_digital initial)
{
  switch (initial) {
    case input_digital::released:
    case input_digital::just_released: {
      return input_digital::released;
    }

    case input_digital::pressed:
    case input_digital::just_pressed: {
      return input_digital::pressed;
    }
  }
}

static input_digital input_digital_promote(const input_digital initial,
                                           const bool pressed)
{
  if (pressed) {
    switch (initial) {
      case input_digital::released:
      case input_digital::just_released: {
        return input_digital::just_pressed;
      }

      case input_digital::pressed:
      case input_digital::just_pressed: {
        return input_digital::pressed;
      }
    }
  }
  else {
    switch (initial) {
      case input_digital::pressed:
      case input_digital::just_pressed: {
        return input_digital::just_released;
      }

      case input_digital::released:
      case input_digital::just_released: {
        return input_digital::released;
      }
    }
  }
}

void inputs::update_begin()
{
  std::for_each(
      _keyboard_buttons.begin(), _keyboard_buttons.end(),
      [](input_digital& button) { button = input_digital_promote(button); });
  std::for_each(
      _mouse_buttons.begin(), _mouse_buttons.end(),
      [](input_digital& button) { button = input_digital_promote(button); });

  _mouse_cursor_delta = input_analog{0.0F, 0.0F};
}

void inputs::update_end() {}

void inputs::update_keyboard_button(const SDL_Scancode scancode,
                                    const bool pressed)
{
  input_digital& button{gsl::at(_keyboard_buttons, scancode)};
  button = input_digital_promote(button, pressed);
}

void inputs::update_mouse_button(const Uint8 button_index, const bool pressed)
{
  input_digital& button{gsl::at(_mouse_buttons, button_index)};
  button = input_digital_promote(button, pressed);
}

void inputs::update_mouse_cursor(const input_analog& absolute,
                                 const input_analog& relative)
{
  _mouse_cursor = absolute;
  _mouse_cursor_delta = relative;
}

input_digital inputs::get_keyboard_button(SDL_Scancode scancode) const
{
  return gsl::at(_keyboard_buttons, scancode);
}

input_digital inputs::get_mouse_button(Uint8 button_index) const
{
  return gsl::at(_mouse_buttons, button_index);
}

input_analog inputs::get_mouse_cursor_absolute() const
{
  return _mouse_cursor;
}

input_analog inputs::get_mouse_cursor_relative() const
{
  return _mouse_cursor_delta;
}
}  // namespace eely