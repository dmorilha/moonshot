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
  screen.dimensions_.offset = screen.dimensions_.surfaceHeight - screen.dimensions_.lineHeight;

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

void Screen::resize(int32_t width, int32_t height) {
  dimensions();
  dimensions_.surfaceWidth = width;
  dimensions_.surfaceHeight = height;
  opengl::clear(dimensions_.surfaceWidth, dimensions_.surfaceHeight, color::black);
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

void Screen::changeScrollY(const int32_t value) {
  int64_t scrollY = 0;
  // screen top might be more than what framebuffer contains
  if (0 < dimensions_.screenTop) {
    scrollY = static_cast<int64_t>(dimensions_.scrollY) + -2 * value;
    if (0 > scrollY) {
      scrollY = 0;
    } else if (scrollY >= dimensions_.screenTop) {
      scrollY = dimensions_.screenTop - 1;
    }
  } else {
    dimensions_.scrollY = 0;
  }
  assert(0 <= scrollY);
  if (dimensions_.scrollY != scrollY) {
    dimensions_.scrollY = static_cast<uint32_t>(scrollY);
  }
  repaint_ = SCROLL;
}

void Screen::draw() {
  if (0 == dimensions_.surfaceHeight || 0 == dimensions_.surfaceWidth) {
    return;
  }
}

Rectangle Screen::printCharacter(Framebuffer::Draw & drawer, const Rectangle & rectangle, rune::Rune rune) {
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

  Rectangle target = drawer(rectangle, rune.backgroundColor);

  const float vertex_bottom = framebuffer_.scaleHeight() * (target.y + glyph.top - (dimensions_.glyphDescender + glyph.height));
  const float vertex_left = framebuffer_.scaleWidth() * (target.x + glyph.left);
  const float vertex_right = framebuffer_.scaleWidth() * (target.x + glyph.left + glyph.width);
  const float vertex_top = framebuffer_.scaleHeight() * (target.y + glyph.top - dimensions_.glyphDescender);

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
  // original 
  buffer_.pushBack(std::move(rune));
  repaint_ = DRAW;

  uint16_t height = dimensions_.lineHeight;
  uint16_t width = dimensions_.glyphWidth;
  uint8_t tab = 1;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\a':
      // assert( ! "UNREACHABLE");
      return;

    case L'\b':
      --dimensions_.column;
      dimensions_.cursorLeft -= width;
      return;

    case L'\t': // horizontal tab
      tab = 8 - (dimensions_.column % 8);
      width *= tab;
      rune.character = L' ';
      break;

    case L'\r': // carriage return
      dimensions_.column = 0;
      dimensions_.cursorLeft = 0;
      return;

    case L'\n': // new line
      dimensions_.totalLines += 1;
      if (dimensions_.line < dimensions_.lines() - 1) {
        dimensions_.line += 1;
      }
      dimensions_.cursorTop += height;
      if (0 == dimensions_.offset) {
        dimensions_.screenTop += height;
        repaintFull_ = true;
      } else if (dimensions_.offset < height) {
        dimensions_.screenTop = height - dimensions_.offset;
        dimensions_.offset = 0;
      } else {
        dimensions_.offset -= height;
      }
      return;

    default:
      break;
    }
  }

  const Rectangle rectangle{
    .x = dimensions_.cursorLeft,
    .y = static_cast<int32_t>(dimensions_.cursorTop),
    .width = width,
    .height = height};

  {
    auto drawer = framebuffer_.draw();
    rectangles_.emplace_back(printCharacter(drawer, rectangle, rune));
  }

  dimensions_.cursorLeft += width;
  dimensions_.column += tab;
}

void Screen::repaint() {
  if (0 >= dimensions_.surfaceHeight || 0 >= dimensions_.surfaceWidth) {
    return;
  }

  //TODO: make sure there is no parallel execution here.
  if (NO != repaint_) {
    glViewport(0, 0, dimensions_.surfaceWidth, dimensions_.surfaceHeight);

    if (repaintFull_) {
      opengl::clear(dimensions_.surfaceWidth, dimensions_.surfaceHeight, color::black);
    }

    // framebuffer_.paintFrame(0);
    framebuffer_.repaint(dimensions_.offset, dimensions_.scrollY);
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
  dimensions_.glyphDescender = face.descender();
  dimensions_.lineHeight = face.lineHeight();
  dimensions_.glyphWidth = face.glyphWidth();
}

void Screen::setCursor(uint16_t column, uint16_t line) {
  assert(0 < column);
  assert(dimensions_.columns() > column);
  assert(0 < line);
  assert(dimensions_.lines() > line);
  dimensions_.column = column;
  dimensions_.line = line;
  dimensions_.cursorLeft = dimensions_.columnToPixel(column);
  dimensions_.cursorTop = dimensions_.lineToPixel(line);
}

#if 0
void Screen::setPosition(const uint16_t x, const uint16_t y) {
  assert(x <= dimensions_.surfaceWidth);
  assert(y <= dimensions_.surfaceHeight);
  dimensions_.column = x / dimensions_.glyphWidth;
  dimensions_.cursorLeft = dimensions_.column * dimensions_.glyphWidth;
  dimensions_.line = y / dimensions_.lineHeight + 1;
  dimensions_.cursorTop = dimensions_.screenTop + dimensions_.line * dimensions_.lineHeight;
}
#endif

void Screen::startSelection(uint16_t x, uint16_t y) {
  assert(x <= dimensions_.surfaceWidth);
  assert(y <= dimensions_.surfaceHeight);
  dimensions_.selection.x = x;
  dimensions_.selection.y = y;
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

Rectangle Framebuffer::Draw::operator () (Rectangle rectangle, const Color & color) {
  assert(framebuffer_.height_ >= rectangle.height
      && framebuffer_.width_ >= rectangle.width);
  framebuffer_.update(rectangle);
  glEnable(GL_SCISSOR_TEST);
  glScissor(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
  glClearColor(color.red, color.green, color.blue, color.alpha);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
  return Rectangle{rectangle.x, rectangle.y, rectangle.width, rectangle.height};
}

void Framebuffer::resize(const uint16_t width, const uint16_t height) {
  width_ = width;
  height_ = height;
  container_.clear();
  current_ = container_.end();
}

void Framebuffer::update(Rectangle & rectangle) {
  bool found = false;

  // cache hit, skip a look-up
  if (container_.end() != current_ && current_->area.y <= rectangle.y && current_->area.y + height_ >= rectangle.y + rectangle.height) {
    // grow current framebuffer area height.
    current_->area.height = std::max(current_->area.height, rectangle.y + rectangle.height - current_->area.y);
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
        current_->area.height = std::max(current_->area.height, rectangle.y + rectangle.height - current_->area.y);
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
    const Color color{.red = drand48(), .green = drand48(), .blue = drand48(), .alpha = 1.f};
#else
    const Color color = color::black;
#endif
    current_ = container_.emplace(container_.end(), Entry{
      .framebuffer = opengl::Framebuffer::New(width_, height_, color),
      .area{.x = 0, .y = rectangle.y, .width = rectangle.width, .height = rectangle.height,}});
    current_->framebuffer.bind();
  }
  rectangle.y = height_ - (rectangle.y - current_->area.y) - rectangle.height;
}

void Framebuffer::repaint(const uint16_t offset, uint32_t scroll) {
  assert((0 <= offset && 0 == scroll) || (0 == offset && 0 <= scroll));
  uint32_t y = 0;
  const auto END = container_.rend();
  auto iterator = container_.rbegin();
  for (; END != iterator && height_ >= y + offset; ++iterator) {
    if (iterator->area.height <= scroll) {
      scroll -= iterator->area.height;
      continue;
    }
    const auto read = iterator->framebuffer.read();

    const uint32_t height = 0 < scroll ? iterator->area.height - scroll : iterator->area.height;
    uint16_t difference = 0;

    if (iterator->area.height + y + offset > height_) {
      difference = height_ - iterator->area.height;
    }

    const float vertices[4][4] = {
      // vertex a - left top
      { -1.f, -1.f + (2.f / height_ * (height + offset + difference)), 0, 0 == y ? 1.f : 1.f - (1.f / height_ * (y - difference)), },
      // vertex b - right top
      { 1.f, -1.f + (2.f / height_ * (height + offset + difference)), 1, 0 == y ? 1.f : 1.f - (1.f / height_ * (y - difference)), },
      // vertex c - right bottom
      { 1.f, -1.f + (2.f / height_ * (y + offset)), 1, 1.f - (1.f / height_ * height)},
      // vertex d - left bottom
      { -1.f, -1.f + (2.f / height_ * (y + offset)), 0, 1.f - (1.f / height_ * height)},
    }; 

#if 0
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        std::cout << vertices[i][j] << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
#endif

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
    y += height;
    scroll = 0;
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
  const Rectangle rectangle{
    .x = dimensions_.cursorLeft,
    .y = static_cast<int32_t>(dimensions_.cursorTop),
    .width = dimensions_.glyphWidth,
    .height = dimensions_.lineHeight};

  auto drawer = framebuffer_.draw();

  drawer(rectangle);

  --dimensions_.column;
}

void Screen::clear() {
}
