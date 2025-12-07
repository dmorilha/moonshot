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

  { /* dimensions */
    freetype::Face & face = characters_.font().regular();
    dimensions_.reset(face, width, height);
  }

  { /* history */
    history_.resize(dimensions_.columns(), dimensions_.lines());
  }

  if (0 < history_.active_size()) {
    resetScroll();
    recreateFromActiveHistory();
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
  const uint64_t new_value = dimensions_.scroll_y() + value;
  if (0 < value) /* if we are scrolling up */  {
    if (new_value + dimensions_.surface_height() >= pages_.total_height()) {
      const uint64_t index = pages_.front_index();
      if (1 < index) {
        recreateFromScrollback(index - 1);
      }
    }
  }
  if (0 < value || dimensions_.scroll_y() > value * -1) {
    dimensions_.scroll_y(new_value);
    repaint_ = FULL;
  } else if (0 < dimensions_.scroll_y()) {
    dimensions_.scroll_y(0);
    repaint_ = FULL;
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

  if (FULL != repaint_ && 0 < dimensions_.scroll_y()) {
    resetScroll();
    repaint_ = FULL;
  } else if (NO == repaint_) {
    repaint_ = PARTIAL;
  }

  switch (rune.character) {
  case L'\0':
    assert(!"UNRECHEABLE");
    return;

  case L'\a': // bell
    return;

  case L'\b': // backspace
    dimensions_.cursor_column(column() - 1);
    history_.move_cursor_backward(1);
    return;

  case L'\r': // carriage return
    dimensions_.cursor_column(1);
    history_.carriage_return();
    return;

  case L'\n': // new line
    if (dimensions_.new_line()) {
      repaint_ = FULL;
    }
    history_.new_line();
    return;
  }

  const uint16_t columns = pushCharacter(rune);
  history_.emplace(std::move(rune));
  dimensions_.cursor_column(column() + columns);
}

uint16_t Screen::pushCharacter(rune::Rune rune) {
  assert(0 < dimensions_.glyph_width());
  uint16_t columns = 1; 
  switch (rune.character) {
  case L'\0':
    rune.character = L' ';
    break;

  case L'\t': // horizontal tab
    columns = 8 - (column() % 8);
    rune.character = L' ';
    break;

  default:
    break;
  }

  Rectangle_Y rectangle = static_cast<Rectangle_Y>(dimensions_);
  rectangle.width = dimensions_.glyph_width() * columns;

  {
    const auto drawer = pages_.draw(rectangle, history_.size());
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

    damage_.emplace(Rectangle{
      .x = drawer.target.x,
      .y = overflow(),
      .width = drawer.target.width,
      .height = drawer.target.height, });
  }

  return columns;
}

//TODO: make sure there is no parallel execution here.
void Screen::repaint(const bool force, const bool alternative) {
  assert(0 < dimensions_.surface_height());
  assert(0 < dimensions_.surface_width());

  if (force || NO != repaint_) {
    opengl::clear(dimensions_.surface_width(), dimensions_.surface_height(), colors::black);
#if 1
    int32_t height = static_cast<int32_t>(dimensions_.line_to_pixel(dimensions_.displayed_lines() + 1));
    int64_t offset_y = 0;
    if (0 < dimensions_.scrollback_lines()) {
      offset_y = dimensions_.scrollback_lines() * dimensions_.line_height();
    }

    if (0 != dimensions_.scroll_y()) {
      offset_y -= dimensions_.scroll_y();
      height = dimensions_.surface_height();
    } else if (dimensions_.overflow()) {
      offset_y -= dimensions_.remainder();
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

    if (alternative) {
      draw_cursor(dimensions_.scroll_y());
    }

    const bool forceSwapBuffers = force || damage_.empty() || FULL == repaint_;
    swapBuffers(forceSwapBuffers);
  }
  repaint_ = NO;
}

void Screen::swapBuffers(bool fullSwap) {
  fullSwap |= damage_.area() * 2 >= dimensions_.area();
  if ( ! fullSwap && ! damage_.empty()) {
    std::vector<Rectangle> rectangles;
    damage_.transfer(rectangles);
    surface_->egl().swapBuffers(rectangles);
  } else {
    surface_->egl().swapBuffers();
  }
  damage_.clear();
}

void Screen::move_cursor(const int column, const int line) {
  assert(0 < column);
  assert(columns() >= column);
  assert(0 < line);
  assert(lines() >= line);
  dimensions_.move_cursor(column, line);
  history_.move_cursor(column, line);
}

void Screen::reverse_line_feed() {
  const uint16_t line = Screen::line() - 1;
  assert(lines() > line);
  if (0 < line) {
    dimensions_.move_cursor(column(), line);
    history_.move_cursor(column(), line);
  } else {
    std::cerr << __FILE__ << ":" << __LINE__ << " (" << __func__ << ") scrolling has not been implemented here." << std::endl;
  }
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

Pages::Drawer Pages::draw(Rectangle_Y rectangle, const uint64_t index) {
  assert(height_ >= rectangle.height);
  assert(width_ >= rectangle.width);
  Entry & entry = update(rectangle, index);
  return Drawer(*this, entry, static_cast<Rectangle>(rectangle));
}

void Pages::reset(const uint16_t width, const uint16_t height) {
  width_ = width;
  height_ = height;
  container_.clear();
  current_ = container_.end();
}

Pages::Entry & Pages::update(Rectangle_Y & rectangle, const uint64_t index) {
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
    current_ = container_.emplace(container_.end(), new_entry(rectangle, index));
  }
  rectangle.y = height_ - (rectangle.y - current_->area.y) - rectangle.height;

  assert(container_.end() != current_);
  return *current_;
}

void Pages::repaint(const Rectangle rectangle, const int64_t offset_y, const bool alt) {
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

  const int64_t a = offset_y + y;
  const int64_t b = offset_y + height;
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
  assert(!"UNRECHEABLE");
  const auto drawer = pages_.draw(static_cast<Rectangle_Y>(dimensions_), 0);
  drawer.clear(colors::black);
#if 0
  damage_.emplace(Rectangle{
    .x = drawer.target.x,
    .y = overflow(),
    .width = drawer.target.width,
    .height = drawer.target.height,});
#endif
  repaint_ = PARTIAL;
}

void Screen::erase_line_right() {
  history_.erase_line_right();
  Rectangle_Y rectangle(dimensions_);
  rectangle.width = dimensions_.surface_width() - rectangle.x;
  const auto drawer = pages_.draw(rectangle, 0);
  drawer.clear(colors::black);
#if 0
  damage_.emplace(Rectangle{
    .x = drawer.target.x,
    .y = overflow(),
    .width = drawer.target.width,
    .height = drawer.target.height,});
#endif
  repaint_ = PARTIAL;
}

void Screen::erase(const int n) {
  assert(0 < n);
  int32_t width = 0;
  history_.erase(n);
  const uint16_t column = Screen::column(),
        end = dimensions_.columns(),
        difference = end - column;
  for (int i = 0; difference >= i; ++i) {
    const rune::Rune & rune = history_.at(column + i, line());
    const uint16_t columns = pushCharacter(rune);
    dimensions_.cursor_column(dimensions_.cursor_column() + columns);
  }
  dimensions_.cursor_column(column);
  repaint_ = PARTIAL;
}

void Screen::insert(const int n) {
  assert(0 < n);
  int32_t width = 0;
  history_.insert(n);
  const uint16_t column = Screen::column(),
        end = dimensions_.columns(),
        difference = end - column;
  for (int i = 0; difference >= i; ++i) {
    const rune::Rune & rune = history_.at(column + i, line());
    const uint16_t columns = pushCharacter(rune);
    dimensions_.cursor_column(dimensions_.cursor_column() + columns);
  }
  dimensions_.cursor_column(column);
  repaint_ = PARTIAL;
}

void Screen::move_cursor_forward(const int n) {
  const uint16_t column = Screen::column() + n;
  if (dimensions_.columns() >= column) {
    history_.move_cursor_forward(n);
    dimensions_.cursor_column(column);
  } else {
    assert(!"UNRECHEABLE");
  }
}

void Screen::move_cursor_backward(const int n) {
  const uint16_t column = Screen::column() - n;
  if (0 < column) {
    history_.move_cursor_backward(n);
    dimensions_.cursor_column(column);
  } else {
    assert(!"UNRECHEABLE");
  }
}

void Screen::move_cursor_down(const int n) {
  const uint16_t line = Screen::line() + n;
  if (dimensions_.lines() >= line) {
    history_.move_cursor_down(n);
    dimensions_.cursor_line(line);
  } else {
    assert(!"UNRECHEABLE");
  }
}

void Screen::move_cursor_up(const int n) {
  uint16_t line = Screen::line() - n;
  if (0 == line) {
    line = 1;
  }
  history_.move_cursor_up(n);
  dimensions_.cursor_line(line);
}

void Screen::erase_display() {
  dimensions_.clear();
  history_.erase_display();
  repaint_ = FULL;
}

void Screen::erase_scrollback() {
  history_.erase_scrollback();
}

int32_t Screen::overflow() {
  int32_t result = 0;
  if ( ! dimensions_.overflow()) {
    result = dimensions_.surface_height() - dimensions_.line_to_pixel(line() + 1);
  }
  return result;
}

void Screen::recreateFromScrollback(const uint64_t index) {
  if (0 == index) {
    std::cerr << __FILE__ << ":" << __LINE__ << " " << __func__ << " index is less than or equal to 0." << std::endl;
    return;
  }
  assert(0 < index);
  History::ReverseIterator end = history_.reverse_iterator(index);
  if (L'\n' == *end) {
    ++end;
  } else {
    assert(L'\0' != *end);
  }
  History::ReverseIterator iterator = end;
  const uint64_t lines = history_.count_lines(iterator, history_.rend(), dimensions_.lines());
  if (L'\n' == *iterator || L'\0' == *iterator) {
    --iterator;
  }
  const int32_t page_size = lines * dimensions_.line_height();
  Pages::Entry & page = pages_.emplace_front(page_size);
  page.index = std::distance(iterator, history_.rend());
  page.framebuffer.bind();
  Rectangle target{
    .x = 0,
    .y = dimensions_.surface_height() - dimensions_.line_height(),
    .width = dimensions_.glyph_width(),
    .height = dimensions_.line_height(),
  };
  int16_t cursor_column = 1;
  for (uint16_t columns = 1; end <= iterator; --iterator) {
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
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Screen::recreateFromActiveHistory() {
  assert(0 < history_.active_size());
  const int32_t pageSize = dimensions_.lines() * dimensions_.line_height();
  Pages::Entry & page = pages_.emplace_front(pageSize);
  page.index = history_.scrollback_size();
  page.framebuffer.bind();
  Rectangle target{
    .x = 0,
    .y = dimensions_.surface_height() - dimensions_.line_height(),
    .width = dimensions_.glyph_width(),
    .height = dimensions_.line_height(),
  };
  const uint16_t columns = history_.columns();
  uint16_t column = 1, last_column = 1, last_line = 1, stride = 1;
  for (uint16_t i = 1; dimensions_.lines() >= i; ++i) {
    for (uint16_t j = 1; columns > j; ++j) {
      target.width = dimensions_.glyph_width();
      const uint32_t index = (i - 1) * columns + j;
      rune::Rune rune = history_.at(index);
      if (rune.iscontrol()) {
        switch (rune.character) {
        case L'\n':
          break;

        case L'\0':
          break;

        case L'\t': // horizontal tab
          stride = 8 - (column % 8);
          if (columns < column + stride) {
            stride = columns - column;
          }
          target.width *= stride;
          rune.character = L' ';
          break;

        default:
          std::cerr << static_cast<int>(rune.character) << std::endl;
          assert(!"UNIMPLEMENTED");
          break;
        }
      }
      if (static_cast<bool>(rune.character)) {
        if (i >= last_line) {
          last_line = i;
          last_column = j + stride;
        }
#if 1
        glEnable(GL_SCISSOR_TEST);
        target(glScissor);
        rune.backgroundColor(glClearColor);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
#endif
        renderCharacter(target, rune);
      }
      target.x += target.width;
      column += stride;
    }
    column = 1;
    target.y -= dimensions_.line_height();
    target.x = 0;
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  repaint_ = FULL;

  // assert(0 <= target.y);
  // assert(dimensions_.lines() >= lines);
  // assert(dimensions_.surface_width() >= target.x);

  const auto cursor = history_.get_cursor();
  std::cout << cursor.first << ", " << cursor.second << std::endl;
  dimensions_.displayed_lines(cursor.second);
  dimensions_.move_cursor(cursor.first + 1, cursor.second);
}

Pages::Entry & Pages::emplace_front(const int32_t height) {
  assert(0 < height);
  assert(0 < height_);
  assert(height <= height_);
  int64_t y = 0;
  if (!container_.empty()) {
    const Entry & front = container_.front();
    y = Pages::front_y() - height;
  }

  const Rectangle_Y rectangle{
    .x = 0,
    .y = y,
    .width = width_,
    .height = height,
  };

  Pages::Entry & page = container_.emplace_front(new_entry(rectangle, 0));

  if (1 == container_.size()) {
    current_ = container_.begin();
  }

  return page;
}

Pages::Entry Pages::new_entry(const Rectangle_Y & rectangle, const uint64_t index) {
  assert(0 < width_);
  assert(0 < height_);
#if 0
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
    .index = index,
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

void Screen::draw_cursor(const uint64_t offset) const {
  Rectangle target{static_cast<Rectangle>(dimensions_)};
  if (0 < offset) {
    return;
  }
  glEnable(GL_SCISSOR_TEST);
  target(glScissor);
  colors::white(glClearColor);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

void Damage::emplace(Rectangle && r) {
  if (container_.empty()) {
    container_.emplace(std::move(r));
    return;
  }
#if 1 // turn optimization on/off
  auto iterator = container_.lower_bound(r);
  if (container_.end() == iterator) {
    --iterator;
  }
  if (iterator->overlaps(r)) {
    if (iterator->contains(r)) {
      return;
    }
    const bool incorporated = r.incorporate(*iterator);
    if (incorporated) {
      container_.erase(iterator);
    }
  }
#endif
  container_.emplace(std::move(r));
}

uint32_t Damage::area() const {
  uint32_t area = 0;
  for (const auto & item : container_) {
    area += item.area();
  }
  return area;
}

void Screen::alternative(const bool mode) {
  history_.alternative(mode);
  if (mode) {
    dimensions_.clear();
    repaint_ = FULL;
  } else {
    /* maybe it can be optimized */
    resize(dimensions_.surface_width(), dimensions_.surface_height());
  }
}
