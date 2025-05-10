#include <array>
#include <iostream>

#include <sys/mman.h>

#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "wayland.h"

namespace wayland {
EGL::~EGL() {
  if (nullptr != eglWindow_) {
    wl_egl_window_destroy(eglWindow_);
    eglWindow_ = nullptr;
  }

  if (nullptr != surface_) {
    assert(nullptr != display_);
    eglDestroySurface(display_, surface_);
    surface_ = nullptr;
  }

  if (nullptr != context_) {
    assert(nullptr != display_);
    eglDestroyContext(display_, context_);
    context_ = nullptr;
  }
}

EGL::EGL(EGL && other) {
  std::swap(context_, other.context_);
  std::swap(display_, other.display_);
  std::swap(surface_, other.surface_);
  std::swap(eglWindow_, other.eglWindow_);
}

EGL & EGL::operator = (EGL && other) {
  std::swap(context_, other.context_);
  std::swap(display_, other.display_);
  std::swap(surface_, other.surface_);
  std::swap(eglWindow_, other.eglWindow_);
  return *this;
}

void EGL::resize(std::size_t height, std::size_t width) {
  assert(nullptr != eglWindow_);
  std::cout << __func__ << " " << width << "x" << height << std::endl;
  return wl_egl_window_resize(eglWindow_, width, height, 0, 0);
}

Surface::Surface(Surface && other) :
  egl_(std::move(other.egl_)),
  surface_(other.surface_),
  toplevel_(other.toplevel_),
  height_(other.height_),
  width_(other.width_) {
    other.surface_ = nullptr;
    other.toplevel_ = nullptr;
    other.height_ = 0;
    other.width_ = 0;
  }

void Surface::onToplevelConfigure(struct xdg_toplevel *, const int32_t width, const int32_t height, struct wl_array *) {
  if (height != height_ || width != width_) {
    height_ = std::max(height, 0);
    width_ = std::max(width, 0);
    egl_.resize(height, width);
  }
}

void Connection::outputMode(struct wl_output * output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
  outputMode_.height = std::max(0, height);
  outputMode_.width = std::max(0, width);
}

void Connection::capabilities() {
  assert(nullptr != display_);
  registry_ = wl_display_get_registry(display_);
  assert(nullptr != registry_);

  {
    const int result = wl_registry_add_listener(registry_, &Connection::registry_listener, this);
    assert(0 == result);
  }

  {
    const int result = wl_display_roundtrip(display_);
    std::cerr << "number of registered capabilities: " << result << std::endl;
  }

  wl_display_roundtrip(display_);

  assert(nullptr != compositor_);
  surface_ = wl_compositor_create_surface(compositor_);
  // wl_surface_add_listener(surface_, &Connection::surface_listener, this);
  assert(nullptr != surface_);
}

EGL Connection::egl() const {
  EGL result;

  assert(nullptr != display_);
  result.display_ = eglGetDisplay(display_);

  EGLint major, minor;
  if (EGL_TRUE != eglInitialize(result.display_, &major, &minor)) {
    std::cerr << "EGL initialization error." << std::endl;
  }

  EGLConfig configuration;
  EGLint number_config;

  constexpr EGLint attributes[] = {
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE };

  eglChooseConfig(result.display_, attributes, &configuration, 1, &number_config);

  if (EGL_TRUE != eglBindAPI(EGL_OPENGL_API)) {
    std::cerr << "EGL API binding error " << eglGetError() << std::endl;
  }

  {
    constexpr EGLint attributes[] = {
      EGL_CONTEXT_MAJOR_VERSION,
      2,
      EGL_CONTEXT_MINOR_VERSION,
      0,
      EGL_NONE,
    };

    result.context_ = eglCreateContext(result.display_, configuration, EGL_NO_CONTEXT, attributes); 
    if (nullptr == result.context_) {
      std::cerr << "Failed to create EGL context." << std::endl;
    }
  }

  assert(nullptr != surface_);
  result.eglWindow_ = wl_egl_window_create(surface_, outputMode_.width, outputMode_.height);

  {
    constexpr EGLAttrib surfaceAttributes[] = { EGL_NONE };
    result.surface_ = eglCreatePlatformWindowSurface(result.display_, configuration, result.eglWindow_, surfaceAttributes);
  }

  return result;
}

std::unique_ptr<Surface> Connection::surface(EGL && egl) const {
  auto result = std::make_unique<Surface>(std::move(egl));
  assert(nullptr != surface_);
  result->surface_ = xdg_wm_base_get_xdg_surface(wm_base_, surface_);
  xdg_surface_add_listener(result->surface_, &Surface::xdg_surface_listener, result.get());
  result->toplevel_ = xdg_surface_get_toplevel(result->surface_);
  xdg_toplevel_add_listener(result->toplevel_, &Surface::toplevel_listener, result.get());
  wl_surface_commit(surface_);
  assert(nullptr != display_);
  wl_display_roundtrip(display_);
  return result;
}

Connection::~Connection() {
  if (nullptr != xkb_.state) {
    xkb_state_unref(xkb_.state);
    xkb_.state = nullptr;
  }
  if (nullptr != xkb_.keymap) {
    xkb_keymap_unref(xkb_.keymap);
    xkb_.keymap = nullptr;
  }
  if (nullptr != xkb_.context) {
    xkb_context_unref(xkb_.context);
    xkb_.context = nullptr;
  }
  if (nullptr != pointer_) {
    wl_pointer_destroy(pointer_);
    pointer_ = nullptr;
  }
  if (nullptr != keyboard_) {
    wl_keyboard_destroy(keyboard_);
    keyboard_ = nullptr;
  }
  if (nullptr != display_) {
    wl_display_disconnect(display_);
    display_ = nullptr;
  }
}

void Connection::connect() {
  if (nullptr != display_) {
    Connection::~Connection();
  }
  display_ = wl_display_connect(nullptr);
  running_ = true;
}

void Connection::registryGlobal(struct wl_registry * const registry,
    const uint32_t name,
    const std::string interface,
    const uint32_t version) {
  // COMPOSITOR
  if (interface == wl_compositor_interface.name && 4 <= version) {
    compositor_ = static_cast<wl_compositor *>(wl_registry_bind(registry_, name, &wl_compositor_interface, 4));

    
  // XDG_SHELL
  } else if (interface == xdg_wm_base_interface.name && 2 <= version) {
    wm_base_ = static_cast<struct xdg_wm_base *>(wl_registry_bind(registry_, name, &xdg_wm_base_interface, 2));

  // OUTPUT
  } else if (interface == wl_output_interface.name && 2 <= version) {
    output_ = static_cast<struct wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, 2));
    wl_output_add_listener(output_, &Connection::output_listener, this);

  // SEAT
  } else if (interface == wl_seat_interface.name && 5 <= version) {
    seat_ = static_cast<struct wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 5));
    wl_seat_add_listener(seat_, &Connection::seat_listener, this);
  }
}

void Connection::seatCapabilities(struct wl_seat * const seat, const uint32_t capabilities) {
  assert(nullptr != seat);
  if (0 != WL_SEAT_CAPABILITY_POINTER & capabilities) {
    assert(nullptr == pointer_);
    pointer_ = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(pointer_, &pointer_listener, this);
  }
  if (0 != WL_SEAT_CAPABILITY_KEYBOARD & capabilities) {
    assert(nullptr == keyboard_);
    keyboard_ = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(keyboard_, &keyboard_listener, this);
  }
}

void Connection::keyboardKeymap(struct wl_keyboard * keyboard,
    uint32_t format,
    int fd,
    uint32_t size) {
  xkb_.context = xkb_context_new(static_cast<xkb_context_flags>(0));
  assert(nullptr != xkb_.context);
  assert(0 < fd);
  char * const map = static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
  assert(MAP_FAILED != map);
  xkb_.keymap = xkb_keymap_new_from_string(xkb_.context, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(static_cast<void *>(map), size);
  assert(nullptr != xkb_.keymap);
  close(fd);

  // should be a class
  xkb_.state = xkb_state_new(xkb_.keymap);
  assert(nullptr != xkb_.state);
}

/*
 * there is a list of todos here
 * what happens when the key is pressed and is not released?
 * international punctuation marks are not working.
 * the current onKeyPress is ugly
 * utf-8 handling smells
 */
void Connection::keyboardKey(struct wl_keyboard * keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
  const uint32_t code = key + 8;
  const enum xkb_key_direction direction =
    (WL_KEYBOARD_KEY_STATE_PRESSED == state) ?
    XKB_KEY_DOWN : XKB_KEY_UP;

  xkb_state_update_key(xkb_.state, code, direction);

  if (WL_KEYBOARD_KEY_STATE_PRESSED == state) {
    std::array<char, 10> buffer{'\0'};
    xkb_state_key_get_utf8(xkb_.state, code, buffer.data(), buffer.size());
    if (static_cast<bool>(onKeyPress)) {
      onKeyPress(buffer.data());
    }
  }
}

int Connection::fd() const {
  assert(nullptr != display_);
  const int fd = wl_display_get_fd(display_);
  assert(0 <= fd);
  return fd;
}

void Connection::roundtrip() const {
  assert(nullptr != display_);
  wl_display_roundtrip(display_);
}
} //end of namespace wayland
