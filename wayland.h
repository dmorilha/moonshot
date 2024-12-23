#pragma once

#include <functional>
#include <string>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>

#include <cassert>

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <xkbcommon/xkbcommon.h>

#include "xdg-shell.h"

namespace wayland {
struct EGL {
  ~EGL();
  EGL() = default;

  EGL(EGL &&);
  EGL(const EGL &) = delete;
  EGL & operator = (EGL &&);
  EGL & operator = (const EGL &) = delete;

  void resize(std::size_t, std::size_t);

// private:
  EGLContext context_ = nullptr;
  EGLDisplay display_ = nullptr;
  EGLSurface surface_ = nullptr;
  struct wl_egl_window * eglWindow_ = nullptr;
};

struct Surface {
  ~Surface() { }

  Surface(EGL & egl) : egl_(egl) { }

  Surface(Surface &&) = default;
  Surface & operator = (Surface &&) = default;

  inline std::size_t height() const { return height_; }
  inline std::size_t width() const { return width_; }

// private:
  constexpr static struct xdg_surface_listener xdg_surface_listener{
    .configure{[](void * data,
      struct xdg_surface * surface,
      uint32_t serial) { xdg_surface_ack_configure(surface, serial); }},
  };

  void onToplevelConfigure(struct xdg_toplevel *, const int32_t, const int32_t, struct wl_array *);

  constexpr static struct xdg_toplevel_listener toplevel_listener{
    .configure{[](void * data,
      struct xdg_toplevel * toplevel,
      int32_t width,
      int32_t height,
      struct wl_array * states) {
        Surface * const surface = static_cast<Surface *>(data);
        assert(nullptr != surface);
        surface->onToplevelConfigure(toplevel, width, height, states);
      }},

    .close{[](void * data,
      struct xdg_toplevel * toplevel) { /* UNIMPLEMENTED */ }},
  };

  EGL & egl_;
  struct xdg_surface * surface_ = nullptr;
  struct xdg_toplevel * toplevel_ = nullptr;
  std::size_t height_ = 0;
  std::size_t width_ = 0;
};

//capabilities could be aggregated through multiple inheritance
struct Connection {
  ~Connection();

  EGL egl();
  Surface surface(EGL &);
  void capabilities();
  void connect();
  void loop();
  void roundtrip() const;
  int fd() const;

// private:
  void registryGlobal(struct wl_registry * const registry,
      const uint32_t name,
      const std::string interface,
      const uint32_t version);

  void seatCapabilities(struct wl_seat * const seat,
      const uint32_t capabilities);

  void outputMode(struct wl_output *,
      uint32_t,
      int32_t,
      int32_t,
      int32_t);

  void keyboardKeymap(struct wl_keyboard * keyboard,
      uint32_t format,
      int fd,
      uint32_t size);

  void keyboardKey(struct wl_keyboard * keyboard,
      uint32_t serial,
      uint32_t time,
      uint32_t key,
      uint32_t state);

  constexpr static struct wl_seat_listener seat_listener{
    .capabilities{[](void * data,
      struct wl_seat * seat,
      uint32_t capabilities) {
        Connection * const connection = static_cast<Connection *>(data);
        assert(nullptr != connection);
        connection->seatCapabilities(seat, capabilities);
      }},

    .name{[](void *, struct wl_seat *, const char *){ /* UNIMPLEMENTED */ }},
  };

  constexpr static struct wl_registry_listener registry_listener{
    .global{[](void * const data,
      struct wl_registry * wl_registry,
      uint32_t name,
      const char * interface,
      uint32_t version) {
        Connection * const connection = static_cast<Connection *>(data);
        assert(nullptr != connection);
        connection->registryGlobal(wl_registry, name, interface, version);
      }},
  };

  constexpr static struct wl_output_listener output_listener{
    .geometry{[](void * data,
      struct wl_output * output,
      int32_t x,
      int32_t y,
      int32_t physical_width,
      int32_t physical_height,
      int32_t subpixel,
      const char * make,
      const char * model,
      int32_t transform) { /* UNIMPLEMENTED */ }},

    .mode{[](void * data,
      struct wl_output * output,
      uint32_t flags,
      int32_t width,
      int32_t height,
      int32_t refresh) {
        Connection * const connection = static_cast<Connection *>(data);
        assert(nullptr != connection);
        connection->outputMode(output, flags, width, height, refresh);
      }},

    .done{[](void * data,
      struct wl_output * output) { /* UNIMPLEMENTED */ }},

    .scale{[](void * data,
      struct wl_output * output,
      int32_t factor) { /* UNIMPLEMENTED */ }},
  };

  constexpr static struct wl_pointer_listener pointer_listener{
    .enter{[](void * data,
      struct wl_pointer * pointer,
      uint32_t serial,
      struct wl_surface * surface,
      wl_fixed_t x,
      wl_fixed_t y) { /* UNIMPLEMENTED */ }},

    .leave{[](void * data,
      struct wl_pointer * pointer,
      uint32_t serial,
      struct wl_surface * surface) { /* UNIMPLEMENTED */ }},

    .motion{[](void * data,
      struct wl_pointer * pointer,
      uint32_t serial,
      wl_fixed_t x,
      wl_fixed_t y) { /* UNIMPLEMENTED */ }},

    .button{[](void * data,
      struct wl_pointer * pointer,
      uint32_t serial,
      uint32_t time,
      uint32_t button,
      uint32_t state) { /* UNIMPLEMENTED */ }},

    .axis{[](void * data,
      struct wl_pointer * pointer,
      uint32_t time,
      uint32_t axis,
      wl_fixed_t value) { /* UNIMPLEMENTED */ }},

    .frame{[](void * data,
        struct wl_pointer * pointer) { /* UNIMPLEMENTED */ }},
      
    .axis_source{[](void * data,
        struct wl_pointer * pointer,
        uint32_t axis) { /* UNIMPLEMENTED */  }},

    .axis_stop{[](void * data,
        struct wl_pointer * pointer,
        uint32_t time,
        uint32_t axis) { /* UNIMPLEMENTED */ }},

    .axis_discrete{[](void * data,
        struct wl_pointer * pointer,
        uint32_t axis,
        int32_t discrete) { /* UNIMPLEMENTED */ }},
  };

  constexpr static struct wl_keyboard_listener keyboard_listener{
    .keymap{[](void * data,
      struct wl_keyboard * keyboard,
      uint32_t format,
      int fd,
      uint32_t size) {
        Connection * const connection = static_cast<Connection *>(data);
        assert(nullptr != connection);
        connection->keyboardKeymap(keyboard, format, fd, size);
      }},

    .enter{[](void * data,
      struct wl_keyboard * keyboard,
      uint32_t serial,
      struct wl_surface * surface,
      struct wl_array * keys) { /* UNIMPLEMENTED */ }},

    .leave{[](void * data,
      struct wl_keyboard * keyboard,
      uint32_t serial,
      struct wl_surface * surface) { /* UNIMPLEMENTED */ }},

    .key{[](void * data,
      struct wl_keyboard * keyboard,
      uint32_t serial,
      uint32_t time,
      uint32_t key,
      uint32_t state) {
        Connection * const connection = static_cast<Connection *>(data);
        assert(nullptr != connection);
        connection->keyboardKey(keyboard, serial, time, key, state);
      }},

    .modifiers{[](void * data,
      struct wl_keyboard * keyboard,
      uint32_t serial,
      uint32_t mods_depressed,
      uint32_t mods_latched,
      uint32_t mods_locked,
      uint32_t group) { /* UNIMPLEMENTED */ }},

    .repeat_info{[](void * data,
      struct wl_keyboard * keyboard,
      int32_t rate,
      int32_t delay) { /* UNIMPLEMENTED */ }},
  };

  struct wl_compositor * compositor_ = nullptr;
  struct wl_display * display_ = nullptr;
  struct wl_keyboard * keyboard_ = nullptr;
  struct wl_output * output_ = nullptr;
  struct wl_pointer * pointer_ = nullptr;
  struct wl_registry * registry_ = nullptr;
  struct wl_seat * seat_ = nullptr;
  struct wl_surface * surface_ = nullptr;
  struct xdg_wm_base * wm_base_ = nullptr;

  struct {
    struct xkb_context * context = nullptr;
    struct xkb_keymap * keymap = nullptr;
    struct xkb_state * state = nullptr;

    struct xkb_compose_table * compose_table = nullptr;
    struct xkb_compose_state * compose_state = nullptr;

    xkb_mod_mask_t ctrl_mask;
    xkb_mod_mask_t alt_mask;
    xkb_mod_mask_t shift_mask;
  } xkb_;

  struct {
    std::size_t height = 0;
    std::size_t width = 0;
  } outputMode_;

  bool running_ = false;

  std::function<void (const char *)> onKeyPress;

  friend class EGL;
  friend class Surface;
};
} //end of namespace wayland
