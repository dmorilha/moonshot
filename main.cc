#include <algorithm>
#include <iostream>

#include "freetype.h"
#include "poller.h"
#include "terminal.h"
#include "wayland.h"

static const char* vertex_shader_text =
"#version 120\n"
"attribute vec4 vPos;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    texcoord = vPos.zw;\n"
"    gl_Position = vec4(vPos.xy, 0, 1);\n"
"}\n";

static const char* fragment_shader_text =
"#version 120\n"
"uniform sampler2D texture;\n"
"uniform vec3 background;\n"
"uniform vec3 color;\n"
"varying vec2 texcoord;\n"
"void main()\n"
"{\n"
"    vec3 character = texture2D(texture, texcoord).rgb;\n"
"    gl_FragColor = vec4(mix(background, color, character), 1.0);\n"
"}\n";

template<class PAINT>
struct WaylandPoller : public Events {
  WaylandPoller(wayland::Connection & c, PAINT && p) : Events(POLLIN), connection_(c), paint(p) { }
  void pollin() override;
private:
  PAINT paint;
  wayland::Connection & connection_;
};

template<class F>
void WaylandPoller<F>::pollin() {
  paint();
  connection_.roundtrip();
}

struct {
  std::size_t column = 0;
  std::size_t glyphDescender = 0;
  std::size_t glyphHeight = 0;
  std::size_t glyphWidth = 0;
  std::size_t leftPadding = 10;
  std::size_t row = 0;
  std::size_t surfaceHeight = 0;
  std::size_t surfaceWidth = 0;
  std::size_t topPadding = 10;
  constexpr std::size_t columns() { return std::floor(surfaceWidth / glyphWidth); }
  constexpr std::size_t rows() { return std::floor(surfaceHeight / glyphHeight); }
  constexpr float scaleHeight() { return 2.f / surfaceHeight; }
  constexpr float scaleWidth() { return 2.f / surfaceWidth; }
} dimensions;

int main(int argc, char ** argv) {
  const float background[3] = { 0.4f, 0.2f, 0.5f, };
  const float color[3] = { 0.8f, 0.4f, 1.f, };
  bool repaint = true;
  std::vector<char> buffer;

  int faceSize =  12;
  if (1 < argc) {
    faceSize = std::atoi(argv[1]);
  }

  freetype::Library freetype;
  const auto face = freetype.load("/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf", faceSize);
  dimensions.glyphDescender = face.descender();
  dimensions.glyphHeight = face.lineHeight();
  dimensions.glyphWidth = face.glyphWidth();

  Poller poller;

  auto t = Terminal::New([&](/* on read */const Terminal::Buffer & b){
      std::copy(b.begin(),
          std::find(b.begin(), b.end(), '\0'),
          std::back_inserter(buffer));
      repaint = true;
      });
  const int fd = t->childfd();
  Terminal & terminal = poller.add(fd, std::move(t));

  wayland::Connection connection;
  connection.connect();
  connection.capabilities();
  auto egl = connection.egl();
  auto surface = connection.surface(egl);

  // opengl rendering
  eglMakeCurrent(egl.display_,
      egl.surface_,
      egl.surface_,
      egl.context_);

  {
    const EGLBoolean result = eglSwapBuffers(egl.display_, egl.surface_);
    if (EGL_FALSE == result) {
      std::cerr << "EGL buffer swap failed." << std::endl;
    }
  }

  connection.roundtrip();

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragment_shader);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  const GLint background_location = glGetUniformLocation(program, "background");
  const GLint color_location = glGetUniformLocation(program, "color");
  const GLint texture_location = glGetUniformLocation(program, "texture");
  const GLint vpos_location = glGetAttribLocation(program, "vPos");

  std::size_t cursor_left = 10;
  std::size_t cursor_top = 10;
  poller.add(connection.fd(), std::unique_ptr<Events>(new WaylandPoller(connection, [&]() {
    if (repaint) {
      eglMakeCurrent(egl.display_,
          egl.surface_,
          egl.surface_,
          egl.context_);

      dimensions.surfaceHeight = surface.height();
      dimensions.surfaceWidth = surface.width();
      glViewport(0, 0, dimensions.surfaceWidth, dimensions.surfaceHeight);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      std::size_t left = dimensions.leftPadding;
      std::size_t top = dimensions.topPadding;
      dimensions.column = 0;
      dimensions.row = 0;

      for (const auto c : buffer) {
        const freetype::Glyph glyph = face.glyph(c);

        // ASCII FOR
        switch (c) {
        case 0x9: // HORIZONTAL TAB
          {
            const std::size_t tab = (dimensions.column + 8) % 8;
            cursor_left = left += tab * dimensions.glyphWidth;
            dimensions.column += tab;
          }
          continue;
          break;
        case 0xa: // CARRIAGE RETURN
          continue;
          break;
        case 0xd: // NEW LINE
          cursor_top = top += face.lineHeight();
          cursor_left = left = 10;
          dimensions.column = 0;
          dimensions.row += 1;
          break;
        }

        glEnable(GL_SCISSOR_TEST);
        glScissor(left, surface.height() - top - face.lineHeight(), dimensions.glyphWidth, dimensions.glyphHeight);
        glClearColor(background[0], background[1], background[2], 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        if (13 == c) {
          continue;
        }

        if (left + glyph.width > dimensions.surfaceWidth) {
          cursor_top = top += dimensions.glyphHeight;
          cursor_left = left = 10;
          dimensions.column = 0;
          dimensions.row += 1;
        }

        GLuint texture, vertex_buffer;

        // texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // const float scale[2] = { 2.f / surface.width(), 2.f / surface.height(), };

#if 0
        std::cout << c << " " << glyph.top << " " << glyph.height << " " << dimensions.glyphHeight << std::endl;
#endif

        const float vertices[4][4] = {
          // vertex a - top left
          { -1.f + dimensions.scaleWidth() * left,
            1.f - dimensions.scaleHeight() * (dimensions.glyphHeight - glyph.top + top + dimensions.glyphDescender),
            0, 0, },

          // vertex b - top right
          { -1.f + dimensions.scaleWidth() * (left + glyph.width),
            1.f - dimensions.scaleHeight() * (dimensions.glyphHeight - glyph.top + top + dimensions.glyphDescender),
            1, 0, },

          // vertex c - bottom right
          { -1.f + dimensions.scaleWidth() * (left + glyph.width),
            1.f - dimensions.scaleHeight() * ((dimensions.glyphHeight - glyph.top) + glyph.height + top + dimensions.glyphDescender),
            1, 1, },

          // vertex d - bottom left
          { -1.f + dimensions.scaleWidth() * left,
            1.f - dimensions.scaleHeight() * ((dimensions.glyphHeight - glyph.top) + glyph.height + top + dimensions.glyphDescender),
            0, 1, },
        }; 

        cursor_left = left += face.glyphWidth();
        dimensions.column += 1;

        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glUseProgram(program);
        glUniform1i(texture_location, 0);

        glUniform3fv(background_location, 1, background);
        glUniform3fv(color_location, 1, color);

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(vpos_location, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDeleteBuffers(1, &vertex_buffer);
        glDeleteTextures(1, &texture);
      }

      glEnable(GL_SCISSOR_TEST);
      glScissor(cursor_left, surface.height() - cursor_top - face.lineHeight(), face.glyphWidth(), face.lineHeight());
      glClearColor(background[0], background[1], background[2], 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glDisable(GL_SCISSOR_TEST);

      const EGLBoolean result = eglSwapBuffers(egl.display_, egl.surface_);
      if (EGL_FALSE == result) {
        std::cerr << "EGL buffer swap failed." << std::endl;
      }
      repaint = false;
    }
  })));

  connection.onKeyPress = [&](const char * const key){
    terminal.write(key, strlen(key));
  };

  poller.on();
  poller.poll();

  return 0;
}
