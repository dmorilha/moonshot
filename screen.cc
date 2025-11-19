#include <algorithm>
#include <iostream>
#include <thread>

#include <cassert>

#include <GL/gl.h>

#include "screen.h"
#include "types.h"

namespace {
std::ostream & operator << (std::ostream & o, const Screen::Repaint r) {
  switch (r) {
  case Screen::NO:
    o << "NO";
    break;
  case Screen::PARTIAL:
    o << "PARTIAL";
    break;
  case Screen::FULL:
    o << "FULL";
    break;
  default:
    assert(!"UNRECHEABLE");
    break;
  }
  return o;
}
} // end of annonymous namespace

Screen Screen::New(const wayland::Connection & connection) {
  auto egl = connection.egl();
  std::unique_ptr<wayland::Surface> surface = connection.surface(std::move(egl));

  surface->setTitle("Moonshot");

  Screen screen(std::move(surface));

  screen.makeCurrent();
  screen.swapBuffers();

  connection.roundtrip();

  screen.glProgram_.vertex(
      "#version 120\n"
      "attribute vec4 vpos;\n"
      "varying vec2 texcoord;\n"
      "void main()\n"
      "{\n"
      "    texcoord = vpos.zw;\n"
      "    gl_Position = vec4(vpos.xy, 0, 1);\n"
      "}\n")
    .fragment(
      "#version 120\n"
      "uniform sampler2D texture;\n"
      "uniform vec3 background;\n"
      "uniform vec3 color;\n"
      "varying vec2 texcoord;\n"
      "void main()\n"
      "{\n"
      "    vec3 character = texture2D(texture, texcoord).rgb;\n"
      "    gl_FragColor = vec4(mix(background, color, character), 1.0);\n"
      "}\n")
    .link();

  screen.pages_.glProgram_.vertex(
      "#version 120\n"
      "attribute vec4 vpos;\n"
      "varying vec2 texcoord;\n"
      "void main()\n"
      "{\n"
      "    texcoord = vpos.zw;\n"
      "    gl_Position = vec4(vpos.xy, 0, 1);\n"
      "}\n")
    .fragment(
      "#version 120\n"
      "uniform sampler2D texture;\n"
      "varying vec2 texcoord;\n"
      "void main()\n"
      "{\n"
      "    gl_FragColor = texture2D(texture, texcoord);\n"
      "}\n")
    .link();

  return screen;
}

void Screen::setTitle(const std::string & title) {
  assert(surface_);
  if ( ! title.empty()) {
    surface_->setTitle(title);
  } else {
    std::cerr << __FILE__ << ":" << __LINE__ << " " << __func__ << " empty title is not supported." << std::endl;
  }
}

Screen::Screen(std::unique_ptr<wayland::Surface> && surface) : surface_(std::move(surface)) {
  assert(static_cast<bool>(surface_));
  surface_->onResize = std::bind_front(&Screen::resize, this);
}

void Screen::resize(const uint16_t width, const uint16_t height) {
  assert(0 < width);
  assert(0 < height);

  /**
   * Pages::reset currently breaks if height
   * is anything different than surface_height
   */
  pages_.reset(width, height);

  {
    freetype::Face & face = characters_.font().regular();
    dimensions_.reset(face, width, height);
  }

  const uint64_t bufferSize = history_.size();
  if (0 < bufferSize) {
    resetScroll();
    recreateFromBuffer(bufferSize - 1);
    repaint_ = FULL;
  } else {
    opengl::clear(dimensions_.surface_width(), dimensions_.surface_height(), colors::black);
    swapBuffers();
  }

  if (static_cast<bool>(onResize)) {
    onResize(width, height);
  }
}

Screen::Screen(Screen && other) : surface_(std::move(other.surface_)) { }

void Screen::changeScrollY(int32_t value) {
  value *= -2;
  // prevents unsigned from overflowing
  if ((0 < value || dimensions_.scroll_y() >= -1 * value)
    && dimensions_.lines() < history_.lines()) {
    const uint64_t difference = pages_.total_height()
      - dimensions_.line_to_pixel(dimensions_.displayed_lines())
      - dimensions_.remainder();
    if (difference >= dimensions_.scroll_y()) {
      dimensions_.scroll_y(std::min(difference, value + dimensions_.scroll_y()));
      repaint_ = FULL;
   }
  }
}

void Screen::renderCharacter(const Rectangle & target, const rune::Rune & rune) {
  const Character & character = characters_.retrieve(rune);
  const float vertex_bottom = pages_.scale_height() * (target.y + character.top - (dimensions_.glyph_descender() + character.height));
  const float vertex_left = pages_.scale_width() * (target.x + character.left);
  const float vertex_right = pages_.scale_width() * (target.x + character.left + character.width);
  const float vertex_top = pages_.scale_height() * (target.y + character.top - dimensions_.glyph_descender());

  const float vertices[4][4] = {
    // vertex a - left top
    { -1.f + vertex_left, -1.f + vertex_top, 0, 0, },
    // vertex b - right top
    { -1.f + vertex_right, -1.f + vertex_top, 1, 0, },
    // vertex c - right bottom
    { -1.f + vertex_right, -1.f + vertex_bottom, 1, 1, },
    // vertex d - left bottom
    { -1.f + vertex_left, -1.f + vertex_bottom, 0, 1, },}; 

  GLuint vertex_buffer = 0;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  assert(0 != character.texture);
  glBindTexture(GL_TEXTURE_2D, character.texture);
  glActiveTexture(GL_TEXTURE0 + character.texture);

  {
    auto shader = glProgram_.use();
    shader.bind(glUniform1i, "texture", 0);
    shader.bind(glUniform3fv, "background", 1, rune.backgroundColor);
    shader.bind(glUniform3fv, "color", 1, rune.foregroundColor);
    shader.bind(glEnableVertexAttribArray, "vpos");
    shader.bind(glVertexAttribPointer, "vpos", 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  glDeleteBuffers(1, &vertex_buffer);
  glActiveTexture(GL_TEXTURE0);

  if (rune.crossout) {
    const Rectangle crossout {
      .x = target.x,
      .y = target.y + dimensions_.line_height() / 2,
      .width = target.width,
      .height = 1,
    };
    glEnable(GL_SCISSOR_TEST);
    crossout(glScissor);
    rune.foregroundColor(glClearColor);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
  }

  if (rune.underline) {
    const Rectangle underline {
      .x = target.x,
      .y = target.y + 2,
      .width = target.width,
      .height = 1,
    };
    glEnable(GL_SCISSOR_TEST);
    underline(glScissor);
    rune.foregroundColor(glClearColor);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
  }
}

void Screen::pushBack(rune::Rune && rune) {
  if (dimensions_.wrap_next()) {
    if (dimensions_.new_line()) {
      repaint_ = FULL;
    }
    dimensions_.cursor_column(1);
  }

  if (FULL != repaint_ &&
      0 < dimensions_.scroll_y()) {
    resetScroll();
    repaint_ = FULL;
  } else if (NO == repaint_) {
    repaint_ = PARTIAL;
  }

  uint64_t buffer_index = history_.pushBack(std::move(rune), dimensions_.carriage_return());

  assert(0 < dimensions_.glyph_width());
  uint16_t columns = 1;
  uint16_t width = dimensions_.glyph_width();
  switch (rune.character) {
  case L'\a':
    // assert( ! "UNREACHABLE");
    return;

  case L'\b': // backspace
    assert(1 < dimensions_.cursor_column());
    dimensions_.cursor_column(dimensions_.cursor_column() - 1);
    buffer_index = history_.erase(1);
    return;

  case L'\t': // horizontal tab
    columns = 8 - (dimensions_.cursor_column() % 8);
    width *= columns;
    rune.character = L' ';
    break;

  case L'\r': // carriage return
    dimensions_.set_carriage_return();
    return;

  case L'\n': // new line
    if (dimensions_.new_line()) {
      repaint_ = FULL;
    }
    return;

  default:
    break;
  }

  Rectangle_Y rectangle = dimensions_;
  rectangle.width = width;

  {
    const auto drawer = pages_.draw(rectangle, buffer_index);
    drawer.clear(rune.backgroundColor);
    renderCharacter(drawer.target, rune);

    if (rune::Blink::STEADY != rune.blink) {
      drawer.create_alternative();
      rune.foregroundColor = rune.backgroundColor;
    }

    if (drawer.alternative()) {
      drawer.clear(rune.backgroundColor);
      renderCharacter(drawer.target, rune);
    }

    rectangles_.emplace_back(Rectangle{
      .x = drawer.target.x,
      .y = overflow(),
      .width = drawer.target.width,
      .height = drawer.target.height, });
  }

  dimensions_.cursor_column(dimensions_.cursor_column() + columns);
}

//TODO: make sure there is no parallel execution here.
void Screen::repaint(const bool force, const bool alternative) {
  assert(0 < dimensions_.surface_height());
  assert(0 < dimensions_.surface_width());

  if (force || NO != repaint_) {
    opengl::clear(dimensions_.surface_width(), dimensions_.surface_height(), colors::black);
#if 1
    int32_t height = static_cast<int32_t>(dimensions_.line_to_pixel(dimensions_.displayed_lines() + 1));
    uint64_t offset_y = 0;
    if (0 < dimensions_.scrollback_lines()) {
      offset_y = dimensions_.scrollback_lines() * dimensions_.line_height();

      if (0 < dimensions_.scroll_y()) {
        if (offset_y > dimensions_.scroll_y()) {
          offset_y -= dimensions_.scroll_y();
        } else  {
          offset_y = 0;
        }

        height = dimensions_.surface_height();

        if (0 < pages_.first_y() &&
            pages_.first_y() + 2 * dimensions_.surface_height() > offset_y) {
            const uint64_t index = pages_.first_buffer_index();
            if (1 < index) {
              recreateFromBuffer(index - 1);
            }
        }
      } else if (dimensions_.overflow()) {
        offset_y -= dimensions_.remainder();
      }

    }
    pages_.repaint(
      Rectangle{
        .x = 0,
        .y = 0,
        .width = dimensions_.surface_width(),
        .height = height, },
      offset_y,
      alternative);
#else
    pages_.paint(0);
#endif
    const bool forceSwapBuffers = force || rectangles_.empty() || FULL == repaint_;
    swapBuffers(forceSwapBuffers);
  }
  repaint_ = NO;
}

void Screen::swapBuffers(const bool full) {
  if ( ! full && ! rectangles_.empty()) {
    // it may compress the rectangles
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

void Screen::setCursor(const uint16_t column, const uint16_t line) {
  dimensions_.set_cursor(column, line);
}

void Screen::startSelection(const uint16_t x, const uint16_t y) {
}

void Screen::endSelection() {
}

void Screen::drag(const uint16_t x, const uint16_t y) {
}

void Screen::select(const Rectangle & rectangle) {
}

Pages::Drawer::~Drawer() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Pages::Drawer::Drawer(const Pages & p, Entry & e, Rectangle && r) : pages_(p), entry_(e), target(std::move(r)) {
  entry_.framebuffer.bind();
}

bool Pages::Drawer::alternative() const {
  const bool result = entry_.alternative;
  if (result) {
    entry_.alternative.bind();
  }
  return result;
}

void Pages::Drawer::create_alternative() const {
  if ( ! entry_.alternative) {
    entry_.alternative = entry_.framebuffer.clone(pages_.width_, pages_.height_);
  }
}

uint32_t Pages::total_height() const {
  uint32_t result = 0;
  for (const auto & item : container_) {
    result += item.area.height;
  }
  return result;
}

void Pages::Drawer::clear(const Color & color) const {
  glEnable(GL_SCISSOR_TEST);
  target(glScissor);
  color(glClearColor);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

Pages::Drawer Pages::draw(Rectangle_Y rectangle, const uint64_t buffer_index) {
  assert(height_ >= rectangle.height);
  assert(width_ >= rectangle.width);
  Entry & entry = update(rectangle, buffer_index);
  return Drawer(*this, entry, static_cast<Rectangle>(rectangle));
}

void Pages::reset(const uint16_t width, const uint16_t height) {
  width_ = width;
  height_ = height;
  container_.clear();
  current_ = container_.end();
}

Pages::Entry & Pages::update(Rectangle_Y & rectangle, const uint64_t buffer_index) {
  bool found = false;

  // cache hit, skip a look-up
  if (container_.end() != current_ && current_->area.y <= rectangle.y && current_->area.y + height_ >= rectangle.y + rectangle.height) {
    // grow current page area height.
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

  if ( ! found) { 
    // cap it to the last cap_ pages.
    if (0 < cap_ && container_.size() >= cap_) {
      const auto begin = container_.begin();
      auto end = begin;
      for (int i = 0; i <= container_.size() - cap_; ++i) {
        assert(container_.end() != end);
        ++end;
      }
      container_.erase(begin, end);
    }

    // if it did not find a page, insert a new page at the end.
    current_ = container_.emplace(container_.end(), entry(rectangle, buffer_index));
  }
  rectangle.y = height_ - (rectangle.y - current_->area.y) - rectangle.height;

  assert(container_.end() != current_);
  return *current_;
}

void Pages::repaint(const Rectangle rectangle, const uint64_t offset_y, const bool alt) {
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
        auto read = iterator->framebuffer.read();
        if (alt && iterator->alternative) {
          read = iterator->alternative.read();
        }

        GLuint vertex_buffer = 0;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        {
          auto shader = glProgram_.use();
          shader.bind(glUniform1i, "texture", 0);
          shader.bind(glEnableVertexAttribArray, "vpos");
          shader.bind(glVertexAttribPointer, "vpos", 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
          glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
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
        auto read = iterator->framebuffer.read();
        if (alt && iterator->alternative) {
          read = iterator->alternative.read();
        }

        GLuint vertex_buffer = 0;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        {
          auto shader = glProgram_.use();
          shader.bind(glUniform1i, "texture", 0);
          shader.bind(glEnableVertexAttribArray, "vpos");
          shader.bind(glVertexAttribPointer, "vpos", 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
          glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
      }
      y = y2;
    }
  }
}

bool Pages::paint(const uint16_t frame) {
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

  {
    auto shader = glProgram_.use();
    shader.bind(glUniform1i, "texture", 0);
    shader.bind(glEnableVertexAttribArray, "vpos");
    shader.bind(glVertexAttribPointer, "vpos", 4, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), nullptr);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  return true;
}

void Screen::backspace() {
  const auto drawer = pages_.draw(static_cast<Rectangle_Y>(dimensions_), 0);
  drawer.clear(colors::black);
  rectangles_.emplace_back(Rectangle{
    .x = drawer.target.x,
    .y = overflow(),
    .width = drawer.target.width,
    .height = drawer.target.height,});
  repaint_ = PARTIAL;
}

void Screen::EL() {
  Rectangle_Y rectangle(dimensions_);
  rectangle.width = dimensions_.surface_width() - rectangle.x;
  const auto drawer = pages_.draw(rectangle, 0);
  drawer.clear(colors::black);
  rectangles_.emplace_back(Rectangle{
    .x = drawer.target.x,
    .y = overflow(),
    .width = drawer.target.width,
    .height = drawer.target.height,});
  repaint_ = PARTIAL;
}

void Screen::clear() {
  dimensions_.clear();
  repaint_ = FULL;
}

void Screen::clearScrollback() {
  std::cerr << __FILE__ << ":" << __LINE__ << " " << __func__ << " unimplemented" << std::endl;
}

int32_t Screen::overflow() {
  int32_t result = 0;
  if ( ! dimensions_.overflow()) {
    result = dimensions_.surface_height() - dimensions_.line_to_pixel(dimensions_.cursor_line() + 1);
  }
  return result;
}

uint64_t Screen::countLines(History::ReverseIterator & iterator, const History::ReverseIterator & end, const uint64_t limit) const {
  uint64_t lines = 0; // lines returns zero, only when iterator equals end from the start
  History::ReverseIterator lineBegin = std::find(iterator, end, L'\n');
  while (end != iterator && (0 == limit || limit >= lines)) {
    ++lines;
    uint32_t cursor_column = 1;
    for (History::ReverseIterator line_iterator = lineBegin - 1;
        iterator <= line_iterator; --line_iterator) {
      uint32_t columns = 1;
      if (line_iterator->iscontrol()) {
        switch (line_iterator->character) {
        case L'\n': // new line
          continue;
        case L'\t': // horizontal tab
          columns = 8 - (cursor_column % 8);
          break;
        default:
          std::cerr << static_cast<int>(line_iterator->character) << std::endl;
          assert(!"UNIMPLEMENTED");
          break;
        }
      }
      const bool wrap = dimensions_.columns() < cursor_column;
      if (wrap) {
        cursor_column = 1;
        ++lines;
        if (0 < limit && limit < lines) {
          iterator = line_iterator;
          return limit;
        }
      }
      cursor_column += columns;
    }
    if (limit < lines) {
      return limit;
    }
    iterator = lineBegin;
    if (end != lineBegin) {
      ++lineBegin;
      lineBegin = std::find(lineBegin, end, L'\n');
    }
  }
  return lines;
}

void Screen::recreateFromBuffer(const uint64_t index) {
  if (0 == index) {
    std::cerr << __FILE__ << ":" << __LINE__ << " " << __func__ << " index is less than or equal to 0." << std::endl;
    return;
  }
  assert(0 < index);
  History::ReverseIterator end = history_.reverseIterator(index);
  if (L'\n' == *end) {
    ++end;
  } else {
    assert(L'\0' != *end);
  }
  History::ReverseIterator iterator = end;
  iterator = std::find(end, history_.rend(), '\n');
  const uint64_t lines = countLines(iterator, history_.rend(), dimensions_.lines());
  if (L'\n' == *iterator || L'\0' == *iterator) {
    --iterator;
  }
  const int32_t page_size = lines * dimensions_.line_height();
  assert(dimensions_.surface_height() >= page_size);
  Pages::Entry & page = pages_.emplace_front(page_size);
  page.buffer_index = std::distance(iterator, history_.rend());
  page.framebuffer.bind();
  Rectangle target{
    .x = 0,
    .y = dimensions_.surface_height() - dimensions_.line_height(),
    .width = dimensions_.glyph_width(),
    .height = dimensions_.line_height(),
  };
  int16_t cursor_column = 1;
  for (uint32_t columns = 1; end <= iterator; --iterator) {
    target.width = dimensions_.glyph_width();
    rune::Rune rune = *iterator;
    if (rune.iscontrol()) {
      switch (rune.character) {
      case L'\n': // new line
        cursor_column = 1;
        target.y -= dimensions_.line_height();
        target.x = 0;
        continue;
      case L'\t': // horizontal tab
        columns = 8 - (cursor_column % 8);
        target.width *= columns;
        rune.character = L' ';
        break;
      default:
        std::cerr << static_cast<int>(rune.character) << std::endl;
        assert(!"UNIMPLEMENTED");
        break;
      }
    }
    const bool wrap = dimensions_.columns() < cursor_column;
    if (wrap) {
      cursor_column = 1;
      target.y -= dimensions_.line_height();
      target.x = 0;
    }
#if 1
    glEnable(GL_SCISSOR_TEST);
    target(glScissor);
    rune.backgroundColor(glClearColor);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
#endif
    renderCharacter(target, rune);
    target.x += target.width;
    cursor_column += columns;
  }
  if (pages_.is_current(page)) {
    // still not sure why cursor_line would go beyond the limits here
    dimensions_.displayed_lines(lines);
    setCursor(cursor_column, lines);
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Pages::Entry & Pages::emplace_front(const int32_t height) {
  assert(0 < height);
  assert(0 < height_);
  assert(height <= height_);
  uint64_t y = 0;
  if (!container_.empty()) {
    const Entry & front = container_.front();
    y = Pages::first_y() - height;
    assert(0 <= y);
  }

  const Rectangle_Y rectangle{
    .x = 0,
    .y = y,
    .width = width_,
    .height = height,
  };

  Pages::Entry & page = container_.emplace_front(entry(rectangle, 0));

  if (1 == container_.size()) {
    current_ = container_.begin();
  }

  return page;
}

Pages::Entry Pages::entry(const Rectangle_Y & rectangle, const uint64_t buffer_index) {
  assert(0 < width_);
  assert(0 < height_);
#if 1
  const Color color{
    .red = static_cast<float>(drand48()),
    .green = static_cast<float>(drand48()),
    .blue = static_cast<float>(drand48()),
    .alpha = 1.f, };
#else
  const Color color = colors::black;
#endif
  return Entry{
    .framebuffer = opengl::Framebuffer::New(width_, height_, color),
    .area{
      .x = 0,
      .y = rectangle.y,
      .width = width_, /* currently unused */
      .height = rectangle.height,
    },
    .buffer_index = buffer_index,
    };
}

bool Pages::has_alternative() const {
  bool result = false;
  for (const auto & entry : container_) {
    result |= entry.alternative;
    if (result) {
      break;
    }
  }
  return result;
}

