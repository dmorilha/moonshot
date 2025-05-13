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

void Screen::write() {
  repaint_ = true;
}

void Screen::clear() {
  buffer_.clear();
  repaint_ = true;
}

void Screen::paint() {
  makeCurrent();

  dimensions_.surfaceHeight = surface_->height();
  dimensions_.surfaceWidth = surface_->width();

  glViewport(0, 0, dimensions_.surfaceWidth, dimensions_.surfaceHeight);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  std::size_t left = dimensions_.leftPadding;
  std::size_t top = dimensions_.topPadding;
  dimensions_.column = 0;
  dimensions_.row = 0;

  /* should be bottom up */
  for (auto c : buffer_) {
    const char d = c.character;
    std::size_t width = dimensions_.glyphWidth;

    switch (d) {
      case '\t': // CARRIAGE RETURN
        {
          const std::size_t tab = 8 - dimensions_.column % 8;
          width *= tab;
          c.character = ' ';
        }
        break;
      case '\r': // CARRIAGE RETURN
        continue;
        break;
      case '\n': // NEW LINE
        c.character = '$';
        break;
      default:
        break;
    }

    const freetype::Glyph glyph = face_.glyph(c.character);

    if (wrap_) {
      if (left + glyph.width > dimensions_.surfaceWidth) {
        dimensions_.cursor_top = top += dimensions_.glyphHeight;
        dimensions_.cursor_left = left = 10;
        dimensions_.column = 0;
        dimensions_.row += 1;
      }
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(left, surface_->height() - top - face_.lineHeight(), width, dimensions_.glyphHeight);
    if (false && c.hasBackgroundColor) {
      glClearColor(c.backgroundColor.red, c.backgroundColor.blue, c.backgroundColor.green, c.backgroundColor.alpha); 
    } else {
      glClearColor(background[0], background[1], background[2], 1);
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);


    GLuint texture, vertex_buffer;

    // texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // const float scale[2] = { 2.f / surface.width(), 2.f / surface.height(), };
    // std::cout << c.character << " " << glyph.top << " " << glyph.height << " " << dimensions_.glyphHeight << std::endl;

    const float vertices[4][4] = {
      // vertex a - top left
      { -1.f + dimensions_.scaleWidth() * left,
        1.f - dimensions_.scaleHeight() * (dimensions_.glyphHeight - glyph.top + top + dimensions_.glyphDescender),
        0, 0, },

      // vertex b - top right
      { -1.f + dimensions_.scaleWidth() * (left + glyph.width),
        1.f - dimensions_.scaleHeight() * (dimensions_.glyphHeight - glyph.top + top + dimensions_.glyphDescender),
        1, 0, },

      // vertex c - bottom right
      { -1.f + dimensions_.scaleWidth() * (left + glyph.width),
        1.f - dimensions_.scaleHeight() * ((dimensions_.glyphHeight - glyph.top) + glyph.height + top + dimensions_.glyphDescender),
        1, 1, },

      // vertex d - bottom left
      { -1.f + dimensions_.scaleWidth() * left,
        1.f - dimensions_.scaleHeight() * ((dimensions_.glyphHeight - glyph.top) + glyph.height + top + dimensions_.glyphDescender),
        0, 1, },
    }; 

    dimensions_.cursor_left = left += width;
    dimensions_.column += 1;

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

    switch (d) {
      case '\t': // HORIZONTAL TAB
        {
          const std::size_t tab = 8 - dimensions_.column % 8;
          dimensions_.column += tab;
        }
        break;
      case '\n': // NEW LINE
        dimensions_.cursor_top = top += face_.lineHeight();
        dimensions_.cursor_left = left = 10;
        dimensions_.column = 0;
        dimensions_.row += 1;
        break;
      default:
        break;
    }
  }

  glEnable(GL_SCISSOR_TEST);
  glScissor(dimensions_.cursor_left, surface_->height() - dimensions_.cursor_top - face_.lineHeight(), face_.glyphWidth(), face_.lineHeight());
  glClearColor(background[0], background[1], background[2], 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);

  swapGLBuffers();
}

void Screen::repaint() {
  if (repaint_) {
    paint();
    repaint_ = false;
  }
}

void Screen::dimensions() {
  dimensions_.glyphDescender = face_.descender();
  dimensions_.glyphHeight = face_.lineHeight();
  dimensions_.glyphWidth = face_.glyphWidth();
  dimensions_.surfaceHeight = surface_->height();
  dimensions_.surfaceWidth = surface_->width();
  buffer_.set_columns(dimensions_.columns());
  std::cout << dimensions_ << std::endl;
}

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "column " << d.column
    << ", columns " << d.columns()
    << ", cursor_left " << d.cursor_left
    << ", cursor_top " << d.cursor_top
    << ", glyphDescender " << d.glyphDescender
    << ", glyphHeight " << d.glyphHeight
    << ", glyphWidth " << d.glyphWidth
    << ", leftPadding " << d.leftPadding
    << ", row " << d.row
    << ", rows " << d.rows()
    << ", scaleHeight " << d.scaleHeight()
    << ", scaleWidth " << d.scaleWidth()
    << ", surfaceHeight " << d.surfaceHeight
    << ", surfaceWidth " << d.surfaceWidth
    << ", topPadding " << d.topPadding;
  return o;
}
