#include "eely_app/app.h"

#include "eely_app/asset_map.h"
#include "eely_app/asset_material.h"
#include "eely_app/asset_mesh.h"
#include "eely_app/asset_uniform.h"
#include "eely_app/inputs.h"

#include <eely/time_utils.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <fmt/format.h>

#include <gsl/assert>
#include <gsl/narrow>
#include <gsl/pointers>

#include <imgui.h>
#include <imgui_impl_bgfx.h>
#include <imgui_impl_sdl.h>

#include <SDL.h>
#include <SDL_mouse.h>
#include <SDL_syswm.h>
#include <SDL_video.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace eely {
constexpr bgfx::ViewId bgfx_imgui_view_id{255};
constexpr uint32_t bgfx_reset_flags{BGFX_RESET_MSAA_X16};

app::app(const unsigned int width, const unsigned int height, const std::string& title)
    : _width{width},
      _height{height},
      _timer_ticks_per_s{SDL_GetPerformanceFrequency()},
      _timer_ticks_per_us{_timer_ticks_per_s / s_to_us},
      _timer_sleep_error_ema_us{0},
      _main_loop_running{false}
{
  Expects(width > 0);
  Expects(height > 0);

  // SDL

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error{fmt::format("SDL_Init error: {}", SDL_GetError())};
  }

  _sdl_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 gsl::narrow<int>(width), gsl::narrow<int>(height),
                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (_sdl_window == nullptr) {
    throw std::runtime_error{fmt::format("SDL_CreateWindow error: {}", SDL_GetError())};
  }

  SDL_DisplayMode sdl_display_mode;
  if (SDL_GetCurrentDisplayMode(0, &sdl_display_mode) != 0) {
    throw std::runtime_error{fmt::format("SDL_GetCurrentDisplayMode error: {}", SDL_GetError())};
  }

  if (sdl_display_mode.refresh_rate != 0) {
    _timer_target_frame_time_s = 1.0F / static_cast<float>(sdl_display_mode.refresh_rate);
  }
  else {
    constexpr float frame_rate_fallback = 60.0F;
    _timer_target_frame_time_s = 1.0F / frame_rate_fallback;
  }

  // bgfx

  // `renderFrame` is called to indicated that we want a single-threaded mode
  bgfx::renderFrame();

  SDL_SysWMinfo sdl_window_manager_info;
  SDL_VERSION(&sdl_window_manager_info.version);
  if (SDL_GetWindowWMInfo(_sdl_window, &sdl_window_manager_info) == SDL_FALSE) {
    throw std::runtime_error{fmt::format("SDL_GetWindowWMInfo error: {}", SDL_GetError())};
  }

  bgfx::PlatformData bgfx_platform_data;
#if BX_PLATFORM_OSX
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  bgfx_platform_data.nwh = sdl_window_manager_info.info.cocoa.window;
#else
  static_assert(false, "System is not supported");
#endif

  bgfx::Init bgfx_init;
  bgfx_init.type = bgfx::RendererType::Metal;  // TODO: this is for macos only
  bgfx_init.resolution.width = width;
  bgfx_init.resolution.height = height;
  bgfx_init.resolution.reset = bgfx_reset_flags;
  bgfx_init.platformData = bgfx_platform_data;
  if (!bgfx::init(bgfx_init)) {
    throw std::runtime_error{"bgfx::init error"};
  }

  // ImGui

  ImGui::CreateContext();
  ImGui::StyleColorsLight();

  ImGui_Implbgfx_Init(bgfx_imgui_view_id);

  const bgfx::RendererType::Enum renderer_type{bgfx::getRendererType()};
  switch (renderer_type) {
    case bgfx::RendererType::Enum::Direct3D9:
    case bgfx::RendererType::Enum::Direct3D11:
    case bgfx::RendererType::Enum::Direct3D12: {
      ImGui_ImplSDL2_InitForD3D(_sdl_window);
    } break;

    case bgfx::RendererType::Enum::Metal: {
      ImGui_ImplSDL2_InitForMetal(_sdl_window);
    } break;

    case bgfx::RendererType::Enum::OpenGLES:
    case bgfx::RendererType::Enum::OpenGL: {
      ImGui_ImplSDL2_InitForOpenGL(_sdl_window, bgfx::getInternalData()->context);
    } break;

    case bgfx::RendererType::Enum::Vulkan: {
      ImGui_ImplSDL2_InitForVulkan(_sdl_window);
    } break;

    default: {
      throw std::runtime_error{fmt::format("ImGui_ImplSDL2: renderer is not supported: {}",
                                           bgfx::getRendererName(renderer_type))};
    } break;
  }

  Ensures(_sdl_window != nullptr);
}

app::~app()
{
  // Clear all asset maps manually here before bgfx shutdown,
  // otherwise it's illegal to make calls to bgfx such as `bgfx::destroy`
  _meshes_runtime.clear();
  _materials.clear();
  _uniforms.clear();

  ImGui_ImplSDL2_Shutdown();
  ImGui_Implbgfx_Shutdown();

  bgfx::shutdown();

  if (_sdl_window != nullptr) {
    SDL_DestroyWindow(_sdl_window);
  }

  SDL_Quit();
}

unsigned int app::get_width() const
{
  return _width;
}

unsigned int app::get_height() const
{
  return _height;
}

const inputs& app::get_inputs() const
{
  return _inputs;
}

asset_map<asset_material::key, asset_material>& app::get_materials()
{
  return _materials;
}

asset_map<asset_uniform::key, asset_uniform>& app::get_uniforms()
{
  return _uniforms;
}

asset_map<asset_mesh::runtime_key, asset_mesh>& app::get_meshes_runtime()
{
  return _meshes_runtime;
}

void app::set_mouse_pinning(bool enabled)
{
  if (enabled) {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_SetWindowGrab(_sdl_window, SDL_TRUE);
  }
  else {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_SetWindowGrab(_sdl_window, SDL_FALSE);
  }
}

bool app::get_mouse_pinning() const
{
  return SDL_GetWindowGrab(_sdl_window) == SDL_TRUE;
}

int app::run()
{
  Expects(!_main_loop_running);

  Uint64 update_time_prev_ticks{SDL_GetPerformanceCounter()};

  _main_loop_running = true;
  while (_main_loop_running) {
    const Uint64 iteration_start_time_ticks{SDL_GetPerformanceCounter()};

    handle_system_events();

    const Uint64 uptime_time_current_ticks{SDL_GetPerformanceCounter()};
    const Uint64 update_delta_time_ticks{uptime_time_current_ticks - update_time_prev_ticks};
    const float update_delta_time_s{static_cast<float>(update_delta_time_ticks) /
                                    static_cast<float>(_timer_ticks_per_s)};
    update_time_prev_ticks = uptime_time_current_ticks;
    update_internal(update_delta_time_s);

    const Uint64 iteration_end_time_ticks{SDL_GetPerformanceCounter()};
    const Uint64 iteration_duration_ticks{iteration_end_time_ticks - iteration_start_time_ticks};
    const float iteration_duration_s{static_cast<float>(iteration_duration_ticks) /
                                     static_cast<float>(_timer_ticks_per_s)};
    const float sleep_time_s{std::max(0.0F, _timer_target_frame_time_s - iteration_duration_s)};
    wait(sleep_time_s);
  }

  Ensures(!_main_loop_running);

  return 0;
}

void app::handle_system_events()
{
  const ImGuiIO& imgui_io{ImGui::GetIO()};

  _inputs.update_begin();

  SDL_Event sdl_event;
  while (SDL_PollEvent(&sdl_event) != 0) {
    ImGui_ImplSDL2_ProcessEvent(&sdl_event);

    switch (sdl_event.type) {
      case SDL_WINDOWEVENT: {
        switch (sdl_event.window.event) {
          case SDL_WINDOWEVENT_RESIZED: {
            _width = sdl_event.window.data1;
            _height = sdl_event.window.data2;

            bgfx::reset(_width, _height, bgfx_reset_flags);
          } break;
        }
      } break;

      case SDL_KEYDOWN: {
        if (!imgui_io.WantCaptureKeyboard) {
          _inputs.update_keyboard_button(sdl_event.key.keysym.scancode, true);
        }
      } break;

      case SDL_KEYUP: {
        if (!imgui_io.WantCaptureKeyboard) {
          _inputs.update_keyboard_button(sdl_event.key.keysym.scancode, false);
        }
      } break;

      case SDL_MOUSEBUTTONDOWN: {
        if (!imgui_io.WantCaptureMouse) {
          _inputs.update_mouse_button(sdl_event.button.button, true);
        }
      } break;

      case SDL_MOUSEBUTTONUP: {
        if (!imgui_io.WantCaptureMouse) {
          _inputs.update_mouse_button(sdl_event.button.button, false);
        }
      } break;

      case SDL_MOUSEMOTION: {
        if (!imgui_io.WantCaptureMouse) {
          const input_analog absolute{static_cast<float>(sdl_event.motion.x),
                                      static_cast<float>(sdl_event.motion.y)};
          const input_analog relative{static_cast<float>(sdl_event.motion.xrel),
                                      static_cast<float>(sdl_event.motion.yrel)};
          _inputs.update_mouse_cursor(absolute, relative);
        }
      } break;

      case SDL_QUIT: {
        _main_loop_running = false;
      } break;
    }
  }

  _inputs.update_end();
}

void app::update(float /*delta_s*/) {}

void app::update_internal(const float delta_s)
{
  if (_inputs.get_keyboard_button(SDL_SCANCODE_ESCAPE) == input_digital::just_pressed) {
    set_mouse_pinning(false);
  }

  if (_inputs.get_mouse_button(SDL_BUTTON_LEFT) == input_digital::just_pressed) {
    set_mouse_pinning(true);
  }

  ImGui_Implbgfx_NewFrame();
  ImGui_ImplSDL2_NewFrame(_sdl_window);
  ImGui::NewFrame();

  update(delta_s);

  ImGui::Render();
  ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

  bgfx::frame();
}

void app::wait(const float duration_s)
{
  // Wait for specified duration more accurately than just
  // `std::this_thread::sleep_for(duration_s)`
  //   - Calculate sleeping error
  //     (i.e. when requested to sleep for X we actually sleep for X + Error)
  //   - Sleeping error is calculated as exponential moving average to smooth out anomalies
  //   - After sleep is done just busy wait last X microseconds if needed,
  //     trying to avoid sudden oversleeping

  if (duration_s <= 0.0F) {
    return;
  }

  constexpr int busy_wait_expected_duration_us{100};
  constexpr float sleep_error_ema_alpha{0.1F};

  const int wait_duration_requested_us{static_cast<int>(duration_s * 1000.0F * 1000.0F)};

  const int sleep_duration_corrected_us{wait_duration_requested_us - _timer_sleep_error_ema_us -
                                        busy_wait_expected_duration_us};
  const Uint64 sleep_start_time_ticks{SDL_GetPerformanceCounter()};
  std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration_corrected_us));
  const Uint64 sleep_end_time_ticks{SDL_GetPerformanceCounter()};

  const Uint64 sleep_duration_actual_ticks{sleep_end_time_ticks - sleep_start_time_ticks};
  const int sleep_duration_actual_us{
      static_cast<int>(sleep_duration_actual_ticks / _timer_ticks_per_us)};
  const int sleep_error_us{sleep_duration_actual_us - sleep_duration_corrected_us};
  _timer_sleep_error_ema_us = static_cast<int>(
      static_cast<float>(sleep_error_us) * sleep_error_ema_alpha +
      static_cast<float>(_timer_sleep_error_ema_us) * (1.0F - sleep_error_ema_alpha));

  const Uint64 wait_end_time_ticks{sleep_start_time_ticks +
                                   wait_duration_requested_us * _timer_ticks_per_us};

  while (SDL_GetPerformanceCounter() < wait_end_time_ticks) {
  }
}
}  // namespace eely