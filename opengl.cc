#include <algorithm>
#include <iterator>
#include <regex>

#include <cassert>

#include <GL/gl.h>

#include "opengl.h"

namespace opengl {

Shader::Entry::~Entry() {
  std::abort();
  if (0 != shader_) {
    glDeleteShader(shader_);
    shader_ = 0;
  }
}

Shader::Entry::Entry(const std::string & text) : text_(text) {
  assert(!text_.empty());
}

void Shader::Entry::compile(const GLenum type) {
  assert(0 == shader_);
  shader_ = glCreateShader(type);
  assert(0 != shader_);
  const GLchar * const pointer = text_.c_str();
  glShaderSource(shader_, 1, &pointer, nullptr);
  glCompileShader(shader_);
}

bool Shader::Location::operator == (const std::string & identifier) const {
  return identifier_ == identifier;
}

std::mutex Shader::Use::gpu_mutex_;

Shader::Use::~Use() {
  glUseProgram(0);
  gpu_mutex_.unlock();
  assert(nullptr != instance_);
}

Shader::Use::Use(const Shader * const instance) : instance_(instance) {
  assert(nullptr != instance_);
  gpu_mutex_.lock();
  glUseProgram(instance_->program_);
}

Shader::~Shader() {
  if (0 != program_) {
    glDeleteProgram(program_);
    program_ = 0;
  }

  for (Entry * const item : shaders_) {
    delete item;
  }
}

Shader & Shader::add(const GLenum type, const std::string & text) {
  assert(0 == program_);
  shaders_.emplace_back(new Entry{text})->compile(type);
  return *this;
}

void Shader::link() {
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  static const std::regex words{"[^\\s]+"};
  static const std::sregex_iterator END;
  assert(0 == program_);
  program_ = glCreateProgram();
  assert(0 != program_);
  for (auto & item : shaders_) {
    assert(0 != item->shader_);
    glAttachShader(program_, item->shader_);
  }
  glLinkProgram(program_);
  glValidateProgram(program_);
  
  enum {
    INITIAL,
    ATTRIBUTE_1,
    ATTRIBUTE_2,
    UNIFORM_1,
    UNIFORM_2,
  } state = INITIAL;
  for (const auto & item : shaders_) {
    assert(!item->text_.empty());
    auto iterator = std::sregex_iterator(item->text_.begin(), item->text_.end(), words);
    for (; END != iterator; ++iterator) {
      switch (state) {
      case INITIAL:
        if ("attribute" == iterator->str()) {
          state = ATTRIBUTE_1;
        } else if ("uniform" == iterator->str()) {
          state = UNIFORM_1;
        }
        break;
      case ATTRIBUTE_1:
        state = ATTRIBUTE_2;
        break;
      case UNIFORM_1:
        state = UNIFORM_2;
        break;
      case ATTRIBUTE_2:
      case UNIFORM_2:
        {
          const std::string identifier = iterator->str().substr(0, iterator->str().length() - 1);
          GLint location = -1;
          switch (state) {
          case ATTRIBUTE_2:
            location = glGetAttribLocation(program_, identifier.c_str());
            break;
          case UNIFORM_2:
            location = glGetUniformLocation(program_, identifier.c_str());
            break;
          default:
            assert(!"UNRECHEABLE");
            break;
          }
          assert(-1 != location);
          if (locations_.end() != std::find(locations_.begin(), locations_.end(), identifier)) {
            std::cerr << "Identifier \"" << identifier << "\" already exists. Aborting execution." << std::endl;
            std::abort();
          }
          locations_.emplace_back(Location{identifier, location});
          state = INITIAL;
        } break;
      default:
        assert(!"UNRECHEABLE");
        break;
      }
    }
  }
}

Shader::Use Shader::use() const {
  assert(0 != program_);
  return Use(this);
}

Framebuffer::Draw::~Draw() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Framebuffer::Draw::Draw(const GLuint framebuffer, const GLuint texture) {
  assert(0 != framebuffer);
  assert(0 != texture);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
  glBindTexture(GL_TEXTURE_2D, texture);
}

Framebuffer::Read::~Read() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

Framebuffer::Read::Read(const GLuint framebuffer, const GLuint texture) {
  assert(0 != framebuffer);
  assert(0 != texture);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
  glBindTexture(GL_TEXTURE_2D, texture);
}

Framebuffer::~Framebuffer() {
  if (0 != framebuffer_) {
    assert(0 != texture_);
    glDeleteTextures(1, &texture_);
    glDeleteFramebuffers(1, &framebuffer_);
  }
  framebuffer_ = texture_ = 0;
}

Framebuffer::Framebuffer(Framebuffer && other) {
  operator =(std::move(other));
}

Framebuffer & Framebuffer::operator = (Framebuffer && other) {
  std::swap(framebuffer_, other.framebuffer_);
  std::swap(texture_, other.texture_);
  return *this;
}

Framebuffer Framebuffer::New(const GLsizei width, const GLsizei height, const Color & color) {
  Framebuffer result;
  glGenFramebuffers(1, &(result.framebuffer_));
  assert(0 != result.framebuffer_);
  glGenTextures(1, &(result.texture_));
  assert(0 != result.texture_);
  glBindFramebuffer(GL_FRAMEBUFFER, result.framebuffer_);
  glBindTexture(GL_TEXTURE_2D, result.texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.texture_, 0);
  opengl::clear(width, height, color);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return result;
}

Framebuffer Framebuffer::clone(const GLsizei width, const GLsizei height) const {
  Framebuffer result = Framebuffer::New(width, height, colors::black);
  Read reader = read();
  Draw drawer = result.draw();
  glCopyPixels(0, 0, width, height, GL_COLOR);
  return result;
}

void Framebuffer::bind() const {
  assert(0 != framebuffer_);
  assert(0 != texture_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glBindTexture(GL_TEXTURE_2D, texture_);
}

Framebuffer::Draw Framebuffer::draw() const {
  return Framebuffer::Draw(framebuffer_, texture_);
}

Framebuffer::Read Framebuffer::read() const {
  return Framebuffer::Read(framebuffer_, texture_);
}

Texture::~Texture() {
  assert(0 != texture_);
  glDeleteTextures(1, &texture_);
}

Texture::Texture() {
  glGenTextures(1, &texture_);
  assert(0 != texture_);
}

void clear(const GLsizei width, const GLsizei height, const Color & color) {
  glViewport(0, 0, width, height);
  glClearColor(color.red, color.green, color.blue, color.alpha);
  glClear(GL_COLOR_BUFFER_BIT);
}

} // end of namespace opengl
