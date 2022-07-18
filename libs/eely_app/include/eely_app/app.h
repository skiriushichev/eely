#pragma once

#include "eely_app/asset_map.h"
#include "eely_app/asset_material.h"
#include "eely_app/asset_mesh.h"
#include "eely_app/asset_uniform.h"
#include "eely_app/inputs.h"

#include <gsl/pointers>

#include <SDL.h>

#include <string>

namespace eely {
// Represents an application base for tools and samples.
class app {
public:
  // Create application with specified window size and title.
  explicit app(unsigned int width, unsigned int height, const std::string& title);

  app(const app&) = delete;
  app(app&&) = delete;

  virtual ~app();

  app& operator=(const app&) = delete;
  app& operator=(app&&) = delete;

  // Return current window width.
  [[nodiscard]] unsigned int get_width() const;

  // Return current window height.
  [[nodiscard]] unsigned int get_height() const;

  // Return application's input state.
  [[nodiscard]] const inputs& get_inputs() const;

  // Get material assets map.
  [[nodiscard]] asset_map<asset_material::key, asset_material>& get_materials();

  // Get uniforms assets map.
  [[nodiscard]] asset_map<asset_uniform::key, asset_uniform>& get_uniforms();

  // Get runtime meshes assets map.
  [[nodiscard]] asset_map<asset_mesh::runtime_key, asset_mesh>& get_meshes_runtime();

  // Set mouse pinning state.
  // Mouse pinning hides cursor and pins it to window's center every frame.
  void set_mouse_pinning(bool enabled);

  // Return `true` if mouse pinning is enabled.
  [[nodiscard]] bool get_mouse_pinning() const;

  // Run the application and return error code.
  // 0 means no error.
  [[nodiscard]] int run();

private:
  virtual void update(float delta_s);

  void handle_system_events();
  void update_internal(float delta_s);
  void wait(float duration_s);

  gsl::owner<SDL_Window*> _sdl_window;
  unsigned int _width;
  unsigned int _height;
  Uint64 _timer_ticks_per_s;
  Uint64 _timer_ticks_per_us;
  float _timer_target_frame_time_s;
  int _timer_sleep_error_ema_us;
  bool _main_loop_running;
  inputs _inputs;

  asset_map<asset_material::key, asset_material> _materials;
  asset_map<asset_uniform::key, asset_uniform> _uniforms;
  asset_map<asset_mesh::runtime_key, asset_mesh> _meshes_runtime;
};
}  // namespace eely