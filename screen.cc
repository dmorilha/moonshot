#include <GL/gl.h>

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

static const char * const fragment_shader_text2 =
  "#version 120\n"
  "uniform sampler2D texture2;\n"
  "varying vec2 texcoord;\n"
  "void main()\n"
  "{\n"
  "    gl_FragColor = texture2D(texture2, texcoord);\n"
  "}\n";

const float backgroundColor[3] = { 0.f, 0.f, 0.f, };
const float foregroundColor[3] = { 1.f, 1.f, 1.f, };

static GLuint vertex_shader = 0;
static GLuint fragment_shader = 0;
static GLuint fragment_shader2 = 0;

struct {
  GLint background = 0;
  GLint color = 0;
  GLint texture = 0;
  GLint vpos = 0;
  GLint texture2 = 0;
  GLint vpos2 = 0;
} location;

GLuint glProgram2 = 0;

} // end of annonymous namespace

/* should it be a singleton ? */
Screen Screen::New(const wayland::Connection & connection, Font && font) {
  auto egl = connection.egl();
  std::unique_ptr<wayland::Surface> surface = connection.surface(std::move(egl));

  surface->setTitle("Terminal");

  Screen screen(std::move(surface), std::move(font));

  screen.makeCurrent();
  screen.swapBuffers();

  connection.roundtrip();

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, nullptr);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, nullptr);
  glCompileShader(fragment_shader);

  fragment_shader2 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader2, 1, &fragment_shader_text2, nullptr);
  glCompileShader(fragment_shader2);

  screen.glProgram_ = glCreateProgram();

  glAttachShader(screen.glProgram_, vertex_shader);
  glAttachShader(screen.glProgram_, fragment_shader);
  glLinkProgram(screen.glProgram_);

  location.background = glGetUniformLocation(screen.glProgram_, "background");
  location.color = glGetUniformLocation(screen.glProgram_, "color");
  location.texture = glGetUniformLocation(screen.glProgram_, "texture");
  location.vpos = glGetAttribLocation(screen.glProgram_, "vPos");

  glProgram2 = glCreateProgram();

  glAttachShader(glProgram2, vertex_shader);
  glAttachShader(glProgram2, fragment_shader2);
  glLinkProgram(glProgram2);

  location.texture2 = glGetUniformLocation(glProgram2, "texture2");
  location.vpos2 = glGetAttribLocation(glProgram2, "vPos");

  screen.dimensions();

  return screen;
}

void Screen::setTitle(const std::string title) {
  assert(surface_);
  if ( ! title.empty()) {
    surface_->setTitle(title);
  }
}

Screen::~Screen() { }

Screen::Screen(std::unique_ptr<wayland::Surface> && surface, Font && font) : surface_(std::move(surface)), font_(std::move(font)) {
  assert(static_cast<bool>(surface_));
  surface_->onResize = std::bind_front(&Screen::resize, this);
}

void Screen::resize(uint16_t width, uint16_t height) {
  assert(0 < width);
  assert(0 < height);
  dimensions();
  dimensions_.surface_width = width;
  dimensions_.surface_height = height;
  opengl::clear(dimensions_.surface_width, dimensions_.surface_height, color::black);
  surface_->egl().swapBuffers();

  framebuffer_.resize(width, height);

  /* pass the number of columns to the buffer for wrapping purposes */
  if (static_cast<bool>(onResize)) {
    onResize(width, height);
  }

  repaint_ = DRAW;
  repaintFull_ = true;
}

Screen::Screen(Screen && other) : surface_(std::move(other.surface_)), font_(std::move(other.font_)) { }

void Screen::changeScrollY(int32_t value) {
  value *= -2;
  // prevents unsigned from overflowing
  if ((0 < value || dimensions_.scroll_y >= -1 * value)
    && 0 < dimensions_.scrollback_lines) {
    const uint64_t difference = framebuffer_.height()
      - dimensions_.line_to_pixel(dimensions_.displayed_lines)
      - dimensions_.surface_height % dimensions_.line_height;
    if (difference >= dimensions_.scroll_y) {
      dimensions_.scroll_y = std::min(difference, value + dimensions_.scroll_y);
      repaint_ = SCROLL;
    }
  }
}

void Screen::draw() {
  assert(0 < dimensions_.surface_height);
  assert(0 < dimensions_.surface_width);
}

Rectangle Screen::printCharacter(Framebuffer::Draw & drawer, const Rectangle_Y & rectangle, rune::Rune rune) {
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\t':
      rune.character = L' ';
      break;
    default:
      assert(!"UNIMPLEMENTED");
      break;
    }
  }

  freetype::Glyph glyph;

  switch (rune.style) {
  case rune::Style::REGULAR:
    glyph = font_.regular().glyph(rune.character);
    break;
  case rune::Style::BOLD:
    glyph = font_.bold().glyph(rune.character);
    break;
  case rune::Style::ITALIC:
    glyph = font_.italic().glyph(rune.character);
    break;
  case rune::Style::BOLD_AND_ITALIC:
    glyph = font_.boldItalic().glyph(rune.character);
    break;
  }

  const Rectangle target = drawer(rectangle, rune.backgroundColor);

  // assert(0 < dimensions_.glyph_descender);
  const float vertex_bottom = framebuffer_.scale_height() * (target.y + glyph.top - (dimensions_.glyph_descender + glyph.height));
  const float vertex_left = framebuffer_.scale_width() * (target.x + glyph.left);
  const float vertex_right = framebuffer_.scale_width() * (target.x + glyph.left + glyph.width);
  const float vertex_top = framebuffer_.scale_height() * (target.y + glyph.top - dimensions_.glyph_descender);

  const float vertices[4][4] = {
    // vertex a - left top
    { -1.f + vertex_left, -1.f + vertex_top, 0, 0, },
    // vertex b - right top
    { -1.f + vertex_right, -1.f + vertex_top, 1, 0, },
    // vertex c - right bottom
    { -1.f + vertex_right, -1.f + vertex_bottom, 1, 1, },
    // vertex d - left bottom
    { -1.f + vertex_left, -1.f + vertex_bottom, 0, 1, },}; 

  // texture
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // vertices
  GLuint vertex_buffer = 0;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // shaders
  assert(0 < glProgram_);
  glUseProgram(glProgram_);
  assert(0 <= location.texture);
  glUniform1i(location.texture, 0);
  assert(0 <= location.background);
  if (rune.hasBackgroundColor) {
    glUniform3fv(location.background, 1, rune.backgroundColor);
  } else {
    glUniform3fv(location.background, 1, backgroundColor);
  }
  assert(0 <= location.color);
  if (rune.hasForegroundColor) {
    glUniform3fv(location.color, 1, rune.foregroundColor);
  } else {
    glUniform3fv(location.color, 1, foregroundColor);
  }
  assert(0 <= location.vpos);
  glEnableVertexAttribArray(location.vpos);
  glVertexAttribPointer(location.vpos, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteTextures(1, &texture);

  return target;
}

void Screen::pushBack(rune::Rune && rune) {
#if 0
  buffer_.pushBack(std::move(rune));
#endif
  repaint_ = DRAW;

  assert(0 < dimensions_.line_height);
  uint16_t height = dimensions_.line_height;
  assert(0 < dimensions_.glyph_width);
  uint16_t width = dimensions_.glyph_width;
  uint8_t tab = 1;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\a':
      // assert( ! "UNREACHABLE");
      return;

    case L'\b':
      assert(1 < dimensions_.cursor_column);
      --dimensions_.cursor_column;
      return;

    case L'\t': // horizontal tab
      tab = 8 - (dimensions_.cursor_column % 8);
      width *= tab;
      rune.character = L' ';
      break;

    case L'\r': // carriage return
      dimensions_.cursor_column = 1;
      return;

    case L'\n': // new line
      dimensions_.new_line();
      return;

    default:
      break;
    }
  }

  Rectangle_Y rectangle = dimensions_;
  rectangle.width = width;;

  {
    auto drawer = framebuffer_.draw();
    rectangles_.emplace_back(printCharacter(drawer, rectangle, rune));
  }

  dimensions_.cursor_column += tab;
#if 0
  assert(dimensions_.columns() >= dimensions_.cursor_column);
#endif
}

void Screen::repaint() {
  assert(0 < dimensions_.surface_height);
  assert(0 < dimensions_.surface_width);

  //TODO: make sure there is no parallel execution here.
  if (NO != repaint_) {
    glViewport(0, 0, dimensions_.surface_width, dimensions_.surface_height);
    if (repaintFull_) {
      opengl::clear(dimensions_.surface_width, dimensions_.surface_height, color::black);
    }

#if 1
    uint64_t offset_y = 0;
    int32_t height = static_cast<int32_t>(dimensions_.line_to_pixel(dimensions_.displayed_lines + 1));
    if (0 < dimensions_.scrollback_lines) {
      offset_y = dimensions_.scrollback_lines * dimensions_.line_height;
      if (dimensions_.lines() == dimensions_.displayed_lines) {
        offset_y -= dimensions_.surface_height % dimensions_.line_height;
      }
      if (0 < dimensions_.scroll_y) {
        if (offset_y > dimensions_.scroll_y) {
          offset_y -= dimensions_.scroll_y;
        } else  {
          offset_y = 0;
        }
        height = dimensions_.surface_height;
      }
    }
    framebuffer_.repaint(Rectangle{
        .x = 0,
        .y = 0,
        .width = dimensions_.surface_width,
        .height = height,
    }, offset_y);
#else
    framebuffer_.paintFrame(0);
#endif
    swapBuffers();
  }
  repaint_ = NO;
}

void Screen::swapBuffers() {
  if ( ! repaintFull_ && ! rectangles_.empty()) {
    std::vector<Rectangle> rectangles;
    for (auto && item : rectangles_) {
      rectangles.emplace_back(std::move(item));
    }
    surface_->egl().swapBuffers(rectangles);
  } else {
    surface_->egl().swapBuffers();
  }
  rectangles_.clear();
}

void Screen::dimensions() {
  freetype::Face & face = font_.regular();
  dimensions_.glyph_descender = face.descender();
  // assert(0 < dimensions_.glyph_descender);
  dimensions_.line_height = face.lineHeight();
  assert(0 < dimensions_.line_height);
  dimensions_.glyph_width = face.glyphWidth();
  assert(0 < dimensions_.glyph_width);
}

void Screen::setCursor(uint16_t column, uint16_t line) {
  dimensions_.set_cursor(column, line);
}

void Screen::startSelection(uint16_t x, uint16_t y) {
}

void Screen::endSelection() {
}

void Screen::drag(const uint16_t x, const uint16_t y) {
}

void Screen::select(const Rectangle & rectangle) {
}

Framebuffer::Draw::~Draw() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Framebuffer::Draw::Draw(Framebuffer & framebuffer) : framebuffer_(framebuffer) {
  if (framebuffer_.container_.end() != framebuffer_.current_) {
    framebuffer_.current_->framebuffer.bind();
  }
}

uint32_t Framebuffer::height() const {
  uint32_t result = 0;
  for (const auto & item : container_) {
    result += item.area.height;
  }
  return result;
}

Rectangle Framebuffer::Draw::operator () (Rectangle_Y rectangle, const Color & color) {
  assert(framebuffer_.height_ >= rectangle.height);
  assert(framebuffer_.width_ >= rectangle.width);
  framebuffer_.update(rectangle);
  glEnable(GL_SCISSOR_TEST);
  glScissor(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
  glClearColor(color.red, color.green, color.blue, color.alpha);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
  return static_cast<Rectangle>(rectangle);
}

void Framebuffer::resize(const uint16_t width, const uint16_t height) {
  width_ = width;
  height_ = height;
  container_.clear();
  current_ = container_.end();
}

void Framebuffer::update(Rectangle_Y & rectangle) {
  bool found = false;

  // cache hit, skip a look-up
  if (container_.end() != current_ && current_->area.y <= rectangle.y && current_->area.y + height_ >= rectangle.y + rectangle.height) {
    // grow current framebuffer area height.
    const uint64_t new_height = rectangle.y + rectangle.height - current_->area.y;
    if (current_->area.height < new_height) {
      current_->area.height = new_height;
    }
    found = true;

  // cache miss
  } else if ( ! container_.empty()) {
    const Container::const_iterator END = container_.end();
    Container::iterator iterator = container_.begin();
    assert(iterator->area.y <= rectangle.y);
    for (; END != iterator; ++iterator) {
      if (iterator->area.y <= rectangle.y && iterator->area.y + iterator->area.height > rectangle.y) {
        // can't grow a previously full buffer
        assert(iterator->area.y + height_ >= rectangle.y + rectangle.height);
        current_ = iterator;
        const uint64_t new_height = rectangle.y + rectangle.height - current_->area.y;
        if (current_->area.height < new_height) {
          current_->area.height = new_height;
        }
        found = true;
        break;
      }
    }
  }

  // if it did not find a framebuffer, insert a new one.
  if ( ! found) { 
    // append fresh framebuffer
    if (0 < cap_ && container_.size() == cap_) {
      container_.pop_front();
    }
#if DEBUG
    const Color color{
      .red = static_cast<float>(drand48()),
      .green = static_cast<float>(drand48()),
      .blue = static_cast<float>(drand48()),
      .alpha = 1.f};
#else
    const Color color = color::black;
#endif
    current_ = container_.emplace(container_.end(), Entry{
      .framebuffer = opengl::Framebuffer::New(width_, height_, color),
      .area{
        .x = 0,
        .y = rectangle.y,
        .width = width_, /* broken */
        .height = rectangle.height,
        }});
    current_->framebuffer.bind();
  }
  rectangle.y = height_ - (rectangle.y - current_->area.y) - rectangle.height;
}

void Framebuffer::repaint(const Rectangle rectangle, const uint64_t offset_y) {
  assert(0 == rectangle.x);
  assert(width_ >= rectangle.width);
  const auto END = container_.end();
  uint16_t y = 0, height = 0;
  for (auto iterator = container_.begin(); END != iterator; ++iterator) {
    height += iterator->area.height;
    if (rectangle.height < height) {
      height = rectangle.height;
      break;
    }
  }
  if (0 == height) {
    return;
  } else if (height_ < height) {
    height = height_;
  }

  for (auto iterator = container_.begin(); END != iterator; ++iterator) {
    if (offset_y > iterator->area.y && offset_y <= iterator->area.y + iterator->area.height) {
      // from
      const float w1 = 0;
      float w2 = iterator->area.width;
      w2 /= std::min(iterator->area.width, rectangle.width);
      float z1 = height_ - iterator->area.height;
      float z2 = z1 + iterator->area.height - (offset_y - iterator->area.y);
      z1 /= height_;
      z2 /= height_;
      // to
      const uint16_t x1 = iterator->area.x;
      const uint16_t x2 = x1 + std::min(iterator->area.width, rectangle.width);
      const uint16_t y1 = y;
      uint16_t y2 = y1 + iterator->area.height - (offset_y - iterator->area.y);
      const float vertices[4][4] = {
        // vertex a - left top
        { -1 + scale_width() * x1, 1 - scale_height() * y1, w1, z2, },
        // vertex b - right top
        { -1 + scale_width() * x2, 1 - scale_height() * y1, w2, z2, },
        // vertex c - right bottom
        { -1 + scale_width() * x2, 1 - scale_height() * y2, w2, z1, },
        // vertex d - left bottom
        { -1 + scale_width() * x1, 1 - scale_height() * y2, w1, z1, },
      }; 

#if 0
      for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
          std::cout << vertices[i][j] << ". ";
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;
#endif

      {
        const auto read = iterator->framebuffer.read();

        GLuint vertex_buffer = 0;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        assert(0 < glProgram2);
        glUseProgram(glProgram2);

        assert(0 <= location.texture2);
        glUniform1i(location.texture2, 0);

        assert(0 <= location.vpos2);
        glEnableVertexAttribArray(location.vpos2);
        glVertexAttribPointer(location.vpos2, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      }
      y = y2;
    }
  }

  const uint64_t a = offset_y + y;
  const uint64_t b = offset_y + height;
  for (auto iterator = container_.begin(); END != iterator; ++iterator) {
    if (a < iterator->area.y + iterator->area.height && b > iterator->area.y) {
      // from
      const float w1 = 0;
      float w2 = iterator->area.width;
      w2 /= std::min(iterator->area.width, rectangle.width);
      float z1 = 0;
      float z2 = std::min(iterator->area.height, height_ - y);
      z1 /= height_;
      z2 /= height_;
      assert(1 >= z2);
      // to
      const uint16_t x1 = iterator->area.x;
      const uint16_t x2 = x1 + std::min(iterator->area.width, rectangle.width);
      const uint16_t y1 = y;
      uint16_t y2 = y1 + iterator->area.height;
      if (height_ < y2) {
        y2 = height_;
      }
      const float vertices[4][4] = {
        // vertex a - left top
        { -1 + scale_width() * x1, 1 - scale_height() * y1, w1, 1 - z1, },
        // vertex b - right top
        { -1 + scale_width() * x2, 1 - scale_height() * y1, w2, 1 - z1, },
        // vertex c - right bottom
        { -1 + scale_width() * x2, 1 - scale_height() * y2, w2, 1 - z2, },
        // vertex d - left bottom
        { -1 + scale_width() * x1, 1 - scale_height() * y2, w1, 1 - z2, },
      }; 

#if 0
      for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
          std::cout << vertices[i][j] << "; ";
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;
#endif

      {
        const auto read = iterator->framebuffer.read();

        GLuint vertex_buffer = 0;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        assert(0 < glProgram2);
        glUseProgram(glProgram2);

        assert(0 <= location.texture2);
        glUniform1i(location.texture2, 0);

        assert(0 <= location.vpos2);
        glEnableVertexAttribArray(location.vpos2);
        glVertexAttribPointer(location.vpos2, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      }
      y = y2;
    }
  }
}

bool Framebuffer::paintFrame(const uint16_t frame) {
  if (container_.size() <= frame) {
    return false;
  }

  auto iterator = container_.begin();
  for (uint16_t counter = 0; frame > counter; ++counter) {
    ++iterator;
  }
  assert(container_.end() != iterator);

  const auto read = iterator->framebuffer.read();
  const float vertices[4][4] = {
    // vertex a - left top
    { -1, 1, 0, 1, },
    // vertex b - right top
    { 1, 1, 1, 1, },
    // vertex c - right bottom
    { 1, -1, 1, 0, },
    // vertex d - left bottom
    { -1, -1, 0, 0, },
  }; 

  GLuint vertex_buffer = 0;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  assert(0 < glProgram2);
  glUseProgram(glProgram2);

  assert(0 <= location.texture2);
  glUniform1i(location.texture2, 0);

  assert(0 <= location.vpos2);
  glEnableVertexAttribArray(location.vpos2);
  glVertexAttribPointer(location.vpos2, 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  return true;
}

void Screen::backspace() {
  auto drawer = framebuffer_.draw();
  drawer(static_cast<Rectangle_Y>(dimensions_));
}

void Screen::clear() {
  dimensions_.scrollback_lines += dimensions_.displayed_lines - 1;
  dimensions_.displayed_lines = 1;
  repaint_ = DRAW;
  repaintFull_ = true;
}
