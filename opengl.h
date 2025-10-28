#pragma once

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

#include <GLES2/gl2.h>

#include "types.h"

namespace opengl {

struct Shader;

struct Shader {
  struct Entry {
    ~Entry();
    Entry(const std::string &);
    auto compile(const GLenum) -> void;
    const std::string text_;
    GLuint shader_ = 0;
  };

  struct Location {
    std::string identifier_;
    GLint location_ = -1;
    auto operator == (const std::string &) const -> bool;
  };

  struct Use {
    ~Use();
    Use(const Shader * const);
    Use(const Use &) = delete;
    Use & operator = (const Use &) = delete;

    template<typename F, typename ... Args>
    void bind(F && f, const std::string & identifier, Args && ... args) {
      const auto iterator = std::find(
          instance_->locations_.cbegin(),
          instance_->locations_.cend(),
          identifier);
      if (instance_->locations_.cend() == iterator) {
        std::cerr << "Parameter \"" << identifier << "\" not found. Aborting the execution." << std::endl;
        std::abort();
      }
      f(iterator->location_, std::forward<Args>(args)...);
    }

  private:
    const Shader * const instance_ = nullptr;
    static std::mutex gpu_mutex_;
  };

  std::vector<Entry *> shaders_;
  std::vector<Location> locations_;

  ~Shader();
  Shader() = default;
  Shader(const Shader &) = delete;
  Shader(Shader &&);

  Shader & operator = (const Shader &) = delete;
  Shader & operator = (Shader &&) = delete;

  auto add(const GLenum, const std::string &) -> Shader &;
  auto fragment(const std::string & text) -> Shader & { return add(GL_FRAGMENT_SHADER, text); }
  auto link() -> void;
  auto vertex(const std::string & text) -> Shader & { return add(GL_VERTEX_SHADER, text); }
  auto use() const -> Use;

  GLuint program_ = 0;
};

struct Framebuffer {
  struct Draw {
    ~Draw();
  private:
    Draw(const GLuint, const GLuint);
    Draw(Draw &) = delete;
    Draw(Draw &&) = delete;
    auto operator = (Draw &) -> Draw & = delete;
    auto operator = (Draw &&) -> Draw & = delete;
    friend class Framebuffer;
  };

  struct Read {
    ~Read();
  private:
    Read(const GLuint, const GLuint);
    Read(Read &) = delete;
    Read(Read &&) = delete;
    auto operator = (Read &) -> Read & = delete;
    auto operator = (Read &&) -> Read & = delete;
    friend class Framebuffer;
  };

  ~Framebuffer();
  Framebuffer() = default;
  Framebuffer(Framebuffer &&);
  Framebuffer & operator = (Framebuffer &&);

  auto bind() const -> void;
  auto draw() const -> Draw;
  auto read() const -> Read;

  static auto New(const GLsizei, const GLsizei, const Color & color = color::black) -> Framebuffer;

private:
  Framebuffer(Framebuffer &) = delete;
  Framebuffer & operator = (Framebuffer &) = delete;

  GLuint framebuffer_ = 0;
  GLuint texture_ = 0;
};

void clear(const GLsizei, const GLsizei, const Color & color = color::black);

} // end of namespace opengl
