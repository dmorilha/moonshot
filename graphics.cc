#include <iostream>

#include <cassert>

#include <GLES2/gl2.h>

#include "graphics.h"

static const char * const vertex_shader_text =
"#version 120\n"
"attribute vec2 vPos;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vPos, 0.0, 1.0);\n"
"    texcoord = vPos;\n"
"}\n";

static const char * const fragment_shader_text =
"#version 120\n"
"uniform sampler2D texture;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color * texture2D(texture, texcoord).rgb, 1.0);\n"
"}\n";

static const float vertices[4][2] =
{
    { 0.f, 0.f },
    { 1.f, 0.f },
    { 1.f, 1.f },
    { 0.f, 1.f }
};


void Graphics::loadFont(freetype::Face && face){
  freetypeFace_ = std::move(face);
  calculateGeometries();
}

void Graphics::resize(const std::size_t height, const std::size_t width) {
  height_ = std::max(std::size_t{0}, height);
  width_ = std::max(std::size_t{0}, width);
  calculateGeometries();
}

void Graphics::calculateGeometries() {
  if (nullptr == freetypeFace_.face_ || 0 >= height_ || 0 >= width_) {
    return;
  }
  freetype::Glyph glyph = freetypeFace_.glyph('O');
  cell_.height = glyph.height + padding_ * 2;
  cell_.width = glyph.width + padding_ * 2;
  columns_ = width_ / cell_.width;
  rows_ = height_ / cell_.height;
}

void Graphics::draw(const char key) {
  if (0 < std::iswalnum(key)) {
    const freetype::Glyph glyph = freetypeFace_.glyph(key);
    assert(nullptr != glyph.pixels);
  }

  glViewport(0, 0, width_, height_);
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  GLuint texture, program, vertex_buffer;
  GLint mvp_location, vpos_location, color_location, texture_location;

  int x, y;
  char pixels[16 * 16];
  GLuint vertex_shader, fragment_shader;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  for (y = 0;  y < 16;  y++) {
    for (x = 0;  x < 16;  x++)
      pixels[y * 16 + x] = rand() % 256;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 16, 16, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  color_location = glGetUniformLocation(program, "color");
  texture_location = glGetUniformLocation(program, "texture");
  vpos_location = glGetAttribLocation(program, "vPos");

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glUseProgram(program);
  glUniform1i(texture_location, 0);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
      sizeof(vertices[0]), (void *)0);

  glUseProgram(program);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
      sizeof(vertices[0]), (void*) 0);

  int i;
  const float colors[2][3] = {
    { 0.8f, 0.4f, 1.f },
    { 0.3f, 0.4f, 1.f }
  };

  glUniform3fv(color_location, 1, colors[i]);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
