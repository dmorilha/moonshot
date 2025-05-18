#include "screen.h"

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

const float background[3] = { 0.4f, 0.2f, 0.5f, };
const float color[3] = { 0.8f, 0.4f, 1.f, };

static GLuint vertex_shader = 0;
static GLuint fragment_shader = 0;

// C++ question ? at some point this may throw 
Screen Screen::New(const wayland::Connection & connection) {
  auto egl = connection.egl();
  auto surface = connection.surface(std::move(egl));

  Screen result(std::move(surface));

  result.makeCurrent();
  result.swapGLBuffers();

  connection.roundtrip();

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragment_shader);

  result.glProgram_ = glCreateProgram();

  glAttachShader(result.glProgram_, vertex_shader);
  glAttachShader(result.glProgram_, fragment_shader);
  glLinkProgram(result.glProgram_);

  result.location_.texture = glGetUniformLocation(result.glProgram_, "texture");
  result.location_.background = glGetUniformLocation(result.glProgram_, "background");
  result.location_.color = glGetUniformLocation(result.glProgram_, "color");
  result.location_.vpos = glGetAttribLocation(result.glProgram_, "vPos");

  return result;
}

Screen::Screen(std::unique_ptr<wayland::Surface> && surface) : surface_(std::move(surface)) {
  assert(static_cast<bool>(surface_));
  surface_->onResize = std::bind_front(&Screen::resize, this);
}

void Screen::resize(int32_t width, int32_t height) {
  dimensions_.surfaceHeight = surface_->height();
  dimensions_.surfaceWidth = surface_->width();
  if (static_cast<bool>(onResize)) {
    onResize(width, height);
  }
  repaint_ = true;
}

Screen::Screen(Screen && other) : surface_(std::move(other.surface_)) { }

void Screen::makeCurrent() const {
  const auto & egl = surface_->egl();
  const auto r = eglMakeCurrent(egl.display_, egl.surface_, egl.surface_, egl.context_);
}

bool Screen::swapGLBuffers() const {
  static std::size_t it = 0;
  const EGLBoolean result = eglSwapBuffers(surface_->egl().display_,
    surface_->egl().surface_);
  if (EGL_FALSE == result) {
    std::cerr << "EGL buffer swap failed. " << it <<  std::endl;
  }
  ++it;
  return EGL_TRUE == result;
}

void Screen::clear() {
  buffer_.clear();
  repaint_ = true;
}

void Screen::paint() {
  makeCurrent();

  glViewport(0, 0, dimensions_.surfaceWidth, dimensions_.surfaceHeight);
  glClearColor(background[0], background[1], background[2], 0);
  glClear(GL_COLOR_BUFFER_BIT);

  std::size_t left = dimensions_.leftPadding + dimensions_.scrollX;
  std::size_t bottom = dimensions_.topPadding + dimensions_.scrollY + face_.lineHeight() * std::max(static_cast<int>(dimensions_.lines() - buffer_.lines()), 0);
  dimensions_.column = 0;
  dimensions_.line = 0;

  std::size_t remainingLines = dimensions_.lines();

  for (const Buffer::Line & line : buffer_) {
    if (0 == remainingLines) {
      break;
    }

    // do some line scanning if wrapping

    for (auto c /* c is a bad name */ : line) {
      const char d = c.character;
      std::size_t width = dimensions_.glyphWidth; // that forces it to be monospaced.

      switch (d) {
        case '\t': // HORIZONTAL TAB
          {
            const std::size_t tab = 8 - dimensions_.column % 8;
            width *= tab;
          }
          c.character = ' ';
          c.hasBackgroundColor = true;
          c.backgroundColor.red = 0.8f;
          c.backgroundColor.green = 0.4f;
          c.backgroundColor.blue = 1.f;
          c.backgroundColor.alpha = 1.f;
          break;
        case '\r': // CARRIAGE RETURN
          continue;
          break;
        case '\n': // NEW LINE
          c.character = '$';
          c.hasBackgroundColor = true;
          c.backgroundColor.red = 0.8f;
          c.backgroundColor.green = 0.4f;
          c.backgroundColor.blue = 1.f;
          c.backgroundColor.alpha = 1.f;
          c.hasForegroundColor = true;
          c.foregroundColor.red = 0.4f;
          c.foregroundColor.green = 0.2f;
          c.foregroundColor.blue = 0.5f;
          c.foregroundColor.alpha = 1.f;
          break;
        default:
          break;
      }

      const freetype::Glyph glyph = face_.glyph(c.character);

      // background
      glEnable(GL_SCISSOR_TEST);
      glScissor(left, bottom, width, dimensions_.glyphHeight);
      if (c.hasBackgroundColor) {
        glClearColor(c.backgroundColor.red, c.backgroundColor.green, c.backgroundColor.blue, c.backgroundColor.alpha); 
      } else {
        glClearColor(background[0], background[1], background[2], 1);
      }
      glClear(GL_COLOR_BUFFER_BIT);
      glDisable(GL_SCISSOR_TEST);

      // texture
      GLuint texture, vertex_buffer;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      #if 0
      std::cout << c.character << " " << glyph.left << " " << glyph.width << " "
        << glyph.top << " " << glyph.height << " " << dimensions_.glyphHeight << std::endl;
      #endif

      const float vertices[4][4] = {
        // vertex a - top left
        { -1.f + dimensions_.scaleWidth() * (left + glyph.left),
          -1.f + dimensions_.scaleHeight() * (bottom + glyph.top - dimensions_.glyphDescender),
          0, 0, },

        // vertex b - top right
        { -1.f + dimensions_.scaleWidth() * (left + glyph.left + glyph.width),
          -1.f + dimensions_.scaleHeight() * (bottom + glyph.top - dimensions_.glyphDescender),
          1, 0, },

        // vertex c - bottom right
        { -1.f + dimensions_.scaleWidth() * (left + glyph.left + glyph.width),
          -1.f + dimensions_.scaleHeight() * (bottom + glyph.top - (dimensions_.glyphDescender + glyph.height)),
          1, 1, },

        // vertex d - bottom left
        { -1.f + dimensions_.scaleWidth() * (left + glyph.left),
          -1.f + dimensions_.scaleHeight() * (bottom + glyph.top - (dimensions_.glyphDescender + glyph.height)),
          0, 1, },
      }; 

      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      assert(0 < glProgram_);
      glUseProgram(glProgram_);
      assert(0 <= location_.texture);
      glUniform1i(location_.texture, 0);

      assert(0 <= location_.background);
      if (c.hasBackgroundColor) {
        glUniform3fv(location_.background, 1, c.backgroundColor);
      } else {
        glUniform3fv(location_.background, 1, background);
      }

      assert(0 <= location_.color);
      if (c.hasForegroundColor) {
        glUniform3fv(location_.color, 1, c.foregroundColor);
      } else {
        glUniform3fv(location_.color, 1, color);
      }

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      assert(0 <= location_.vpos);
      glEnableVertexAttribArray(location_.vpos);
      glVertexAttribPointer(location_.vpos, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDeleteBuffers(1, &vertex_buffer);
      glDeleteTextures(1, &texture);

      left += width;
      switch (d) {
        case '\t': // HORIZONTAL TAB
          {
            const std::size_t tab = 8 - dimensions_.column % 8;
            dimensions_.column += tab;
          }
          break;
        case '\n': // NEW LINE
          goto nextLine;
        default:
          dimensions_.column += 1;
          break;
      }
    }

nextLine:
    bottom += face_.lineHeight();
    left = dimensions_.leftPadding + dimensions_.scrollX;
    dimensions_.column = 0;
    dimensions_.line -= 1;
    remainingLines--;
  }

  #if 0
  glEnable(GL_SCISSOR_TEST);
  glScissor(dimensions_.cursor_left, surface_->height() - dimensions_.cursor_top - face_.lineHeight(), face_.glyphWidth(), face_.lineHeight());
  glClearColor(background[0], background[1], background[2], 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
  #endif

  swapGLBuffers();
}

void Screen::repaint() {
  if (repaint_) {
    paint();
    repaint_ = false;
  }
}

void Screen::dimensions() {
  dimensions_.glyphAscender = face_.ascender();
  dimensions_.glyphDescender = face_.descender();
  dimensions_.glyphHeight = face_.lineHeight();
  dimensions_.glyphWidth = face_.glyphWidth();
  dimensions_.surfaceHeight = surface_->height();
  dimensions_.surfaceWidth = surface_->width();
  #if 0
  std::cout << dimensions_ << std::endl;
  #endif
}

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "column " << d.column
    << ", columns " << d.columns()
    << ", cursor_left " << d.cursor_left
    << ", cursor_top " << d.cursor_top
    << ", glyphAscender " << d.glyphAscender
    << ", glyphDescender " << d.glyphDescender
    << ", glyphHeight " << d.glyphHeight
    << ", glyphWidth " << d.glyphWidth
    << ", leftPadding " << d.leftPadding
    << ", line " << d.line
    << ", lines " << d.lines()
    << ", scaleHeight " << d.scaleHeight()
    << ", scaleWidth " << d.scaleWidth()
    << ", surfaceHeight " << d.surfaceHeight
    << ", surfaceWidth " << d.surfaceWidth
    << ", topPadding " << d.topPadding;
  return o;
}
