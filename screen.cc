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
  result.swapBuffers();

  connection.roundtrip();

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragment_shader);

  try {
    result.glProgram_ = glCreateProgram();

    glAttachShader(result.glProgram_, vertex_shader);
    glAttachShader(result.glProgram_, fragment_shader);
    glLinkProgram(result.glProgram_);

    result.location_.texture = glGetUniformLocation(result.glProgram_, "texture");
    result.location_.background = glGetUniformLocation(result.glProgram_, "background");
    result.location_.color = glGetUniformLocation(result.glProgram_, "color");
    result.location_.vpos = glGetAttribLocation(result.glProgram_, "vPos");
  } catch (...) {
  }

  return result;
}

Screen::Screen(Screen && other) : surface_(std::move(other.surface_)) { }

void Screen::makeCurrent() const {
  const auto & egl = surface_->egl();
  const auto r = eglMakeCurrent(egl.display_, egl.surface_, egl.surface_, egl.context_);
}

bool Screen::swapBuffers() const {
  static std::size_t it = 0;
  const EGLBoolean result = eglSwapBuffers(surface_->egl().display_,
    surface_->egl().surface_);
  if (EGL_FALSE == result) {
    std::cerr << "EGL buffer swap failed. " << it <<  std::endl;
  }
  ++it;
  return EGL_TRUE == result;
}

void Screen::write(const Buffer & buffer) {
  std::copy(buffer.begin(),
      /* std::find(buffer.begin(), buffer.end(), '\0'), */
      buffer.end(),
      std::back_inserter(buffer_));
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

  for (const auto c : buffer_) {
    const freetype::Glyph glyph = face_.glyph(c.character);

    // ASCII FOR
    switch (c.character) {
      case 0x9: // HORIZONTAL TAB
        {
          const std::size_t tab = (dimensions_.column + 8) % 8;
          cursor_left_ = left += tab * dimensions_.glyphWidth;
          dimensions_.column += tab;
        }
        continue;
        break;
      case 0xa: // CARRIAGE RETURN
        continue;
        break;
      case 0xd: // NEW LINE
        cursor_top_ = top += face_.lineHeight();
        cursor_left_ = left = 10;
        dimensions_.column = 0;
        dimensions_.row += 1;
        break;
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(left, surface_->height() - top - face_.lineHeight(), dimensions_.glyphWidth, dimensions_.glyphHeight);
    if (false && c.hasBackgroundColor) {
      glClearColor(c.backgroundColor.red, c.backgroundColor.blue, c.backgroundColor.green, c.backgroundColor.alpha); 
    } else {
      glClearColor(background[0], background[1], background[2], 1);
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (13 == c) {
      continue;
    }

    if (left + glyph.width > dimensions_.surfaceWidth) {
      cursor_top_ = top += dimensions_.glyphHeight;
      cursor_left_ = left = 10;
      dimensions_.column = 0;
      dimensions_.row += 1;
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

    cursor_left_ = left += face_.glyphWidth();
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
  }

  glEnable(GL_SCISSOR_TEST);
  glScissor(cursor_left_, surface_->height() - cursor_top_ - face_.lineHeight(), face_.glyphWidth(), face_.lineHeight());
  glClearColor(background[0], background[1], background[2], 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);

  swapBuffers();
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
}

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "d.column " << d.column
    << ", d.glyphDescender " << d.glyphDescender
    << ", d.glyphHeight " << d.glyphHeight
    << ", d.glyphWidth " << d.glyphWidth
    << ", d.leftPadding " << d.leftPadding
    << ", d.row " << d.row
    << ", d.surfaceHeight " << d.surfaceHeight
    << ", d.surfaceWidth " << d.surfaceWidth
    << ", d.topPadding " << d.topPadding;
  return o;
}
