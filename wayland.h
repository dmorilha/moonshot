#pragma once

#include <functional>
#include <memory>
#include <string>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <EGL/egl.h>
// #define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include <GLES2/gl2.h>

#include <cassert>

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <xkbcommon/xkbcommon.h>

#include "xdg-shell.h"

namespace wayland {

struct Connection;

struct EGL {
  ~EGL(); 
  EGL() = default;

  EGL(EGL &&);
  EGL(const EGL &) = delete;
  EGL & operator = (EGL &&);
  EGL & operator = (const EGL &) = delete;

  void resize(std::size_t, std::size_t);

  void makeCurrent() const;
  bool swapBuffers() const;

  template <class CONTAINER>
  bool swapBuffers(const wayland::EGL & egl, CONTAINER & container) const {
    assert(nullptr != eglSwapBuffersWithDamageKHR_);
    assert(nullptr != display_);
    assert(nullptr != surface_);
    const EGLBoolean result = eglSwapBuffersWithDamageKHR_(
        display_, surface_, reinterpret_cast<const int *>(container.data()), container.size());
    if (EGL_FALSE == result) {
      std::cerr << "EGL buffer swap failed." << std::endl;
    }
    return EGL_TRUE == result;
  }

  EGLDisplay display() const { return display_; }
  EGLSurface surface() const { return surface_; }

private:
  EGLContext context_ = nullptr;
  EGLDisplay display_ = nullptr;
  EGLSurface surface_ = nullptr;
  struct wl_egl_window * eglWindow_ = nullptr;

  // EGL Extensions
  PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC eglSwapBuffersWithDamageKHR_ = nullptr;

  friend class Connection;
};

struct Surface {
  ~Surface() = default;
  Surface(EGL && egl) : egl_(std::move(egl)) { }

  Surface(const Surface &) = delete;
  Surface(Surface &&);
  Surface & operator = (Surface &) = delete;
  Surface & operator = (Surface &&) = delete;

  uint16_t height() const { return height_; }
  uint16_t width() const { return width_; }
  void setTitle(const std::string &);

  const EGL & egl() const { return egl_; }

  void onToplevelConfigure(struct xdg_toplevel *, const int32_t, const int32_t, struct wl_array *);

  std::function<void (int32_t, int32_t)> onResize;

private:
  EGL egl_;
  struct xdg_surface * surface_ = nullptr;
  struct xdg_toplevel * toplevel_ = nullptr;
  uint16_t height_ = 0;
  uint16_t width_ = 0;

  friend class Connection;
};

//capabilities could be aggregated through multiple inheritance
struct Connection {
  ~Connection();

  EGL egl() const;
  std::unique_ptr<Surface> surface(EGL &&) const;
  void capabilities();
  void connect();
  void loop();
  void roundtrip() const;
  int fd() const;

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

  void pointerButton(struct wl_pointer * pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state);

  void pointerAxis(struct wl_pointer * pointer,
      uint32_t time,
      uint32_t axis,
      int32_t value);

  std::function<void (const uint32_t, const char * const, const size_t, const uint32_t)> onKeyPress;
  std::function<void (uint32_t, int32_t)> onPointerAxis;
  std::function<void (uint32_t, uint32_t)> onPointerButton;

private:
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
    struct xkb_state * clean_state = nullptr;

    struct xkb_compose_table * compose_table = nullptr;
    struct xkb_compose_state * compose_state = nullptr;
  } xkb_;

  struct {
    std::size_t height = 0;
    std::size_t width = 0;
  } outputMode_;

  bool running_ = false;

  friend class EGL;
  friend class Surface;
};
} //end of namespace wayland
