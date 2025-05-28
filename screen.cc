#include "screen.h"
#include "types.h"

namespace {
static const char * const vertex_shader_text =
  "#version 120\n"
  "attribute vec4 vPos;\n"
  "varying vec2 texcoord;\n"
  "void main()\n"
  "{\n"
  "    texcoord = vPos.zw;\n"
  "    gl_Position = vec4(vPos.xy, 0, 1);\n"
  "}\n";

static const char * const fragment_shader_text =
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

const float background[3] = { 0.f, 0.f, 0.f, };
const float color[3] = { 0.f, 1.f, 0.f, };

static GLuint vertex_shader = 0;
static GLuint fragment_shader = 0;

struct {
  GLint background = 0;
  GLint color = 0;
  GLint texture = 0;
  GLint vpos = 0;
} location;
} // end of annonymous namespace

/* should it be a singleton ? */
Screen Screen::New(const wayland::Connection & connection, Font && font) {
  auto egl = connection.egl();
  std::unique_ptr<wayland::Surface> surface = connection.surface(std::move(egl));

  surface->setTitle("Terminal");

  Screen result(std::move(surface), std::move(font));

  result.makeCurrent();
  result.swapBuffers();

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

  location.texture = glGetUniformLocation(result.glProgram_, "texture");
  location.background = glGetUniformLocation(result.glProgram_, "background");
  location.color = glGetUniformLocation(result.glProgram_, "color");
  location.vpos = glGetAttribLocation(result.glProgram_, "vPos");

  result.dimensions();
  result.dimensions_.cursorBottom = (result.dimensions_.scrollY % result.dimensions_.lineHeight) + result.dimensions_.lineHeight * result.dimensions_.lines();
  result.dimensions_.cursorLeft = result.dimensions_.leftPadding + result.dimensions_.scrollX;

  result.paint();

  return result;
}

Screen::Screen(std::unique_ptr<wayland::Surface> && surface, Font && font) : surface_(std::move(surface)), font_(std::move(font)) {
  assert(static_cast<bool>(surface_));
  surface_->onResize = std::bind_front(&Screen::resize, this);
}

void Screen::resize(int32_t width, int32_t height) {
  dimensions();
  /* pass the number of columns to the buffer for wrapping purposes */
  if (static_cast<bool>(onResize)) {
    onResize(width, height);
  }
  repaint_ = true;
}

Screen::Screen(Screen && other) : surface_(std::move(other.surface_)), font_(std::move(other.font_)) { }

void Screen::clear() {
  buffer_.clear();
  repaint_ = true;
}

void Screen::changeScrollY(const int32_t value) {
  const int16_t scrollY = std::min(dimensions_.scrollY + value * 2, 0);
  if (scrollY != dimensions_.scrollY
      && dimensions_.lines() < buffer_.lines() + 1 + (scrollY / dimensions_.lineHeight)) {
    dimensions_.scrollY = scrollY;
    dimensions_.cursorBottom = dimensions_.bottomPadding + (dimensions_.scrollY % dimensions_.lineHeight) + dimensions_.lineHeight * std::max(static_cast<int>(dimensions_.lines() - buffer_.lines()), 0);
    repaint_ = repaintFull_ = true;
  }
}

void Screen::changeScrollX(const int32_t value) {
  const int16_t scrollX = std::min(dimensions_.scrollX + value * 2, 0);
  if (scrollX != dimensions_.scrollX) {
    dimensions_.scrollX = scrollX;
    dimensions_.cursorLeft = dimensions_.leftPadding + dimensions_.scrollX;
    repaint_ = repaintFull_ = true;
  }
}

/*
 by evaluating "wayst" source code, it becomes clear that to gain on
 performace the "screen" has to allow for merges of modified rects.
 */

/* a spot for constant re-evaluation */
void Screen::paint() {
  glViewport(0, 0, dimensions_.surfaceWidth, dimensions_.surfaceHeight);

  /* difference */
  const auto difference = buffer_.difference();
  if ( ! repaintFull_ && ! difference.empty()) {
    for (auto print : difference) {
      uint16_t width = dimensions_.glyphWidth; // that forces it to be monospaced.
      uint8_t tab = 1;
      if (print.iscontrol()) {
        switch (print.character) {
        case L'\t': // HORIZONTAL TAB
          tab = 8 - dimensions_.column % 8;
          width *= tab;
          print.character = L' ';
          break;
        case L'\r': // CARRIAGE RETURN
          dimensions_.column = 0;
          dimensions_.cursorLeft = dimensions_.leftPadding;
          continue;
        case L'\n': // NEW LINE
          ++dimensions_.line;
          dimensions_.cursorBottom -= dimensions_.lineHeight;
          if (dimensions_.lineHeight > dimensions_.cursorBottom) {
            goto repaintFull;
          }
          continue;
        default:
          break;
        }
      }

      rectangles_.emplace_back(Rectangle{dimensions_.cursorLeft, dimensions_.cursorBottom, width, dimensions_.lineHeight});

      freetype::Glyph glyph;
      glyph = font_.regular().glyph(print.character);

      // background
      glEnable(GL_SCISSOR_TEST);
      glScissor(dimensions_.cursorLeft, dimensions_.cursorBottom, width, dimensions_.lineHeight);
      glClearColor(background[0], background[1], background[2], 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glDisable(GL_SCISSOR_TEST);

      // vertices
      GLuint vertex_buffer = 0;
      const float vertex_bottom = -1.f + dimensions_.scaleHeight() * (dimensions_.cursorBottom + glyph.top - (dimensions_.glyphDescender + glyph.height));
      const float vertex_left = -1.f + dimensions_.scaleWidth() * (dimensions_.cursorLeft + glyph.left);
      const float vertex_right = -1.f + dimensions_.scaleWidth() * (dimensions_.cursorLeft + glyph.left + glyph.width);
      const float vertex_top = -1.f + dimensions_.scaleHeight() * (dimensions_.cursorBottom + glyph.top - dimensions_.glyphDescender);

      const float vertices[4][4] = {
        // vertex a - left top
        { vertex_left, vertex_top, 0, 0, },
        // vertex b - right top
        { vertex_right, vertex_top, 1, 0, },
        // vertex c - right bottom
        { vertex_right, vertex_bottom, 1, 1, },
        // vertex d - left bottom
        { vertex_left, vertex_bottom, 0, 1, },
      }; 

      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      // texture
      GLuint texture = 0;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      assert(0 < glProgram_);
      glUseProgram(glProgram_);
      assert(0 <= location.texture);
      glUniform1i(location.texture, 0);

      assert(0 <= location.background);
      glUniform3fv(location.background, 1, background);

      assert(0 <= location.color);
      glUniform3fv(location.color, 1, color);

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      assert(0 <= location.vpos);
      glEnableVertexAttribArray(location.vpos);
      glVertexAttribPointer(location.vpos, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDeleteBuffers(1, &vertex_buffer);
      glDeleteTextures(1, &texture);

      dimensions_.cursorLeft += width;
      dimensions_.column += tab;
    }
    return;
  }

  /* no difference, full repaint */
repaintFull:
  rectangles_.clear();
  repaintFull_ = true;
  glClearColor(background[0], background[1], background[2], 0);
  glClear(GL_COLOR_BUFFER_BIT);
  int16_t cursorBottom = dimensions_.cursorBottom = dimensions_.bottomPadding + (dimensions_.scrollY % dimensions_.lineHeight) + dimensions_.lineHeight * std::max(static_cast<int>(dimensions_.lines() - buffer_.lines()), 0);
  int16_t cursorLeft = dimensions_.leftPadding;
  uint16_t column = dimensions_.column;
  uint16_t line = dimensions_.line;
  uint16_t remainingLines = dimensions_.lines() + 2;
  uint16_t skip = std::abs(dimensions_.scrollY);
  bool firstLine = true;
  const auto END = buffer_.rend();
  for (auto iterator = buffer_.rbegin(); END != iterator; ++iterator) {
    if (0 == remainingLines) {
      break;
    }

    while (dimensions_.lineHeight <= skip) {
      skip -= dimensions_.lineHeight;
      ++iterator;
      assert(END != iterator);
    }

    for (auto rune : *iterator) {
      const char character = rune.character;
      uint16_t width = dimensions_.glyphWidth; // that forces it to be monospaced.

      switch (character) {
      case L'\t': // HORIZONTAL TAB
        {
          const std::size_t tab = 8 - column % 8;
          width *= tab;
        }
        rune.character = L' ';
        break;
      case L'\r': // CARRIAGE RETURN
        continue;
      case L'\n': // NEW LINE
        goto nextLine;
      default:
        break;
      }

      freetype::Glyph glyph;
      switch (rune.style) {
      case rune.BOLD:
        glyph = font_.bold().glyph(rune.character);
        break;
      case rune.BOLD_AND_ITALIC:
        glyph = font_.boldItalic().glyph(rune.character);
        break;
      case rune.ITALIC:
        glyph = font_.italic().glyph(rune.character);
        break;
      case rune.REGULAR:
        glyph = font_.regular().glyph(rune.character);
        break;
      default:
        assert(!"UNREACHEABLE");
        break;
      }

      // background
      glEnable(GL_SCISSOR_TEST);
      glScissor(cursorLeft, cursorBottom, width, dimensions_.lineHeight);
      if (rune.hasBackgroundColor) {
        glClearColor(rune.backgroundColor.red, rune.backgroundColor.green, rune.backgroundColor.blue, rune.backgroundColor.alpha); 
      } else {
        glClearColor(background[0], background[1], background[2], 1);
      }
      glClear(GL_COLOR_BUFFER_BIT);
      glDisable(GL_SCISSOR_TEST);

      #if 0
      std::cout << rune.character << " " << glyph.left << " " << glyph.width << " "
        << glyph.top << " " << glyph.height << " " << dimensions_.lineHeight << std::endl;
      #endif

      // vertices
      const float vertex_bottom = -1.f + dimensions_.scaleHeight() * (cursorBottom + glyph.top - (dimensions_.glyphDescender + glyph.height));
      const float vertex_left = -1.f + dimensions_.scaleWidth() * (cursorLeft + glyph.left);
      const float vertex_right = -1.f + dimensions_.scaleWidth() * (cursorLeft + glyph.left + glyph.width);
      const float vertex_top = -1.f + dimensions_.scaleHeight() * (cursorBottom + glyph.top - dimensions_.glyphDescender);

      const float vertices[4][4] = {
        // vertex a - left top
        { vertex_left, vertex_top, 0, 0, },
        // vertex b - right top
        { vertex_right, vertex_top, 1, 0, },
        // vertex c - right bottom
        { vertex_right, vertex_bottom, 1, 1, },
        // vertex d - left bottom
        { vertex_left, vertex_bottom, 0, 1, },
      }; 

      GLuint texture, vertex_buffer;
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      // texture
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      assert(0 < glProgram_);
      glUseProgram(glProgram_);
      assert(0 <= location.texture);
      glUniform1i(location.texture, 0);

      assert(0 <= location.background);
      if (rune.hasBackgroundColor) {
        glUniform3fv(location.background, 1, rune.backgroundColor);
      } else {
        glUniform3fv(location.background, 1, background);
      }

      assert(0 <= location.color);
      if (rune.hasForegroundColor) {
        glUniform3fv(location.color, 1, rune.foregroundColor);
      } else {
        glUniform3fv(location.color, 1, color);
      }

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      assert(0 <= location.vpos);
      glEnableVertexAttribArray(location.vpos);
      glVertexAttribPointer(location.vpos, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDeleteBuffers(1, &vertex_buffer);
      glDeleteTextures(1, &texture);

      cursorLeft += width;
      switch (character) {
      case L'\t': // HORIZONTAL TAB
        {
          const uint16_t tab = 8 - column % 8;
          column += tab;
        }
        break;
      default:
        column += 1;
        break;
      }
    }

nextLine:
    if (firstLine) {
      dimensions_.cursorLeft = cursorLeft;
      firstLine = false;
    }
    cursorBottom += dimensions_.lineHeight;
    cursorLeft = dimensions_.leftPadding + dimensions_.scrollX;
    column = 0;
    line -= 1;
    remainingLines--;
  }
}

void Screen::pushBack(Rune && rune) {
  buffer_.pushBack(std::move(rune));
  repaint_ = true;
}

void Screen::repaint() {
  if (repaint_) {
    repaint_ = false;
    paint();
    if ( ! repaintFull_ || ! rectangles_.empty()) {
      // std::cout << "*" << std::flush;
      std::vector<Rectangle> rectangles;
      for (auto && item : rectangles_) {
        rectangles.emplace_back(std::move(item));
      }
      surface_->egl().swapBuffers(rectangles);
    } else {
      // std::cout << "-" << std::flush;
      repaintFull_ = false;
      surface_->egl().swapBuffers();
    }
    rectangles_.clear();
  } else {
    // std::cout << "!" << std::flush;
  }
}

void Screen::dimensions() {
  freetype::Face & face = font_.regular();
  dimensions_.glyphAscender = face.ascender();
  dimensions_.glyphDescender = face.descender();
  dimensions_.lineHeight = face.lineHeight();
  dimensions_.glyphWidth = face.glyphWidth();
  dimensions_.surfaceHeight = surface_->height();
  dimensions_.surfaceWidth = surface_->width();
}

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "bottomPadding " << d.bottomPadding
    << ", column " << d.column
    << ", columns " << d.columns()
    << ", cursorBottom " << d.cursorBottom
    << ", cursorLeft " << d.cursorLeft
    << ", glyphAscender " << d.glyphAscender
    << ", glyphDescender " << d.glyphDescender
    << ", glyphWidth " << d.glyphWidth
    << ", leftPadding " << d.leftPadding
    << ", line " << d.line
    << ", lineHeight " << d.lineHeight
    << ", lines " << d.lines()
    << ", scaleHeight " << d.scaleHeight()
    << ", scaleWidth " << d.scaleWidth()
    << ", scrollX " << d.scrollX
    << ", scrollY " << d.scrollY
    << ", surfaceHeight " << d.surfaceHeight
    << ", surfaceWidth " << d.surfaceWidth;
  return o;
}
