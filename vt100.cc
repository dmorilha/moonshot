#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#include <cassert>

#include <errno.h>

#include "screen.h"
#include "vt100.h"

vt100::vt100(Screen & screen) : Terminal(screen) { }

void vt100::handleDecMode(const unsigned int code, const bool mode) {
#if 0
  std::cerr << __func__ << "(" << code << ", " << (mode ? "true" : "false") << ");" << std::endl;
#endif
  switch (code) {
  case 1:
    /* decckm */
    // application keypad cursor
    break;

  case 3:
    /* deccolm */
    assert(!"UNIMPLEMENTED");
    break;

  case 4:
    /* decsclm */
    assert(!"UNIMPLEMENTED");
    break;
 
  case 5:
    /* decscnm */
    assert(!"UNIMPLEMENTED");
    break;

  case 6:
    /* deccom - makes cursor movement relative to the scroll region */
    assert(!"UNIMPLEMENTED");
    break;

  case 7:
    /* decawm - no wrap around */
    assert(!"UNIMPLEMENTED");
    break;

  case 8:
    /* decarm - auto repeat */
    assert(!"UNIMPLEMENTED");
    break;

  case 9:
    /* mouse x10 */
    assert(!"UNIMPLEMENTED");
    break;

  case 10:
    /* show toolbar (rxvt) */
    assert(!"UNIMPLEMENTED");
    break;

  case 12:
    /* start blinking cursor */
    assert(!"UNIMPLEMENTED");
    break;

  case 13:
    /* start blinking cursor (xterm non-standard) */
    assert(!"UNIMPLEMENTED");
    break;

  case 14:
    /* enable xor of blinking cursor control sequence and menu */
    assert(!"UNIMPLEMENTED");
    break;

  case 15:
    /* printer not connected dsr */
    assert(!"UNIMPLEMENTED");
    break;

  case 19:
    /* decpex - don't limit print to scrolling region */
    assert(!"UNIMPLEMENTED");
    break;

  case 25:
    /* dectcem - hide/show cursor */
    /* value = self->cursor.hidden; */
    break;

  case 40:
    /* allow 80 => 132 mode, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 45:
    /* reverse wrap around mode, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 61:
    /* decvccm - vertical cursor coupling mode*/
    assert(!"UNIMPLEMENTED");
    break;

  case 64:
    /* decpccm - page cursor coupling mode*/
    assert(!"UNIMPLEMENTED");
    break;

  case 66:
    /* decnkm - numeric keypad */
    assert(!"UNIMPLEMENTED");
    break;

  case 67:
    /* decbkm - set backarrow key to backspace / delete */
    assert(!"UNIMPLEMENTED");
    break;

  case 68:
    /* deckbum - keyboard usage */
    assert(!"UNIMPLEMENTED");
    break;

  case 69:
    /* decvssm - enable left and right margin mode */
    assert(!"UNIMPLEMENTED");
    break;

  case 1000:
    /* x11 xterm mouse protocol */
    /* value = self->mouse.mouse_vt200; */
    break;

  case 1001:
    /* highlight mouse tracking, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1002:
    /* xterm cell motion mouse tracking */
    assert(!"UNIMPLEMENTED");
    break;

  case 1003:
    /* xterm all motion tracking */
    assert(!"UNIMPLEMENTED");
    break;

  case 1004:
    /* send CSI I and CSI O when it looses focus, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1005:
    /* utf-8 mouse mode */
    assert(!"UNIMPLEMENTED");
    break;

  case 1006:
    /* sgr mouse mode */
    assert(!"UNIMPLEMENTED");
    break;

  case 1015:
    /* urxvt mouse mode */
    assert(!"UNIMPLEMENTED");
    break;

  case 1016:
    /* sgr pixels mouse mode */
    assert(!"UNIMPLEMENTED");
    break;

  case 1034:
    /* xterm eight bit input */
    assert(!"UNIMPLEMENTED");
    break;

  case 1035:
    /* xterm num lock */
    assert(!"UNIMPLEMENTED");
    break;

  case 1036:
    /* xterm meta sends escape */
    assert(!"UNIMPLEMENTED");
    break;

  case 1037:
    /* del sends del */
    assert(!"UNIMPLEMENTED");
    break;

  case 1039:
    /* no alt sends esc */
    assert(!"UNIMPLEMENTED");
    break;

  case 1042:
    /* bell sets urgent wm hint, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1043:
    /* bell raises window pop on bell, xterm */ 
    assert(!"UNIMPLEMENTED");
    break;

  case 47: /* use alternate screen buffer, xterm */
  case 1047: /* use alternate screen buffer, xterm */
  case 1049:
    /* after saving the cursor, switch to the alternate screen buffer, clearing it first */
    if (mode) {
      alternative_buffer_on();
    } else {
      alternative_buffer_off();
    }
    break;

  case 1051:
    /* sun function key mode, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1052:
    /* hp function key mode, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1053:
    /* sco function key mode, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1060:
    /* legacy keyboard emulation, X11R6 */
    assert(!"UNIMPLEMENTED");
    break;

  case 1061:
    /* vt220 keyboard emulation, xterm */
    assert(!"UNIMPLEMENTED");
    break;

  case 1070:
    /* use private color registers for each graphic */
    assert(!"UNIMPLEMENTED");
    break;

  case 2004:
    /* bracketed paste */
    // self->modes.bracketed_paste = mode;
    break;

  case 2026:
    /* application synchronized updates equivalent to BSU/ESU */
    assert(!"UNIMPLEMENTED");
    break;

  case 8452:
    /* sixel scrolling leaves cursor to right of graphic */
    assert(!"UNIMPLEMENTED");
    break;

  default:
    assert(!"UNKNOWN DEC MODE");
    break;
  }
}

void vt100::handleAPC(const char) { }

void vt100::handleSGRCommand(const int command) {
  switch (command) {
  case 0:
    /* normal (default) */
    rune_factory_.reset();
    break;

  case 1:
    /* bold, vt100 */
    rune_factory_.isBold = true;
    break;

  case 2:
    /* faint / dim / decreased intensity, ecma 48 2nd */
    assert(!"UNIMPLEMENTED");
    break;

  case 3:
    /* italicized, ecma 48 2nd */
    rune_factory_.isItalic = true;
    break;

  case 4:
    /* underline */
    rune_factory_.underline = true;
    break;

  case 5:
    /* slow (less than 150 per minute) blink */
    rune_factory_.blink = rune::Blink::SLOW;
    break;

  case 6:
    /* fast blink (ms-dos), most terminals that implement this use the same speed */
    rune_factory_.blink = rune::Blink::FAST;
    break;

  case 7:
    /* inverse colors */
    {
      const Color color = rune_factory_.backgroundColor;
      rune_factory_.backgroundColor = rune_factory_.foregroundColor;
      rune_factory_.foregroundColor = color;
    } break;

  case 8:
    /* invisible, hidden, ecma 48 2nd, vt300 */
    assert(!"UNIMPLEMENTED");
    break;

  case 9:
    /* crossed-out characters, ecma 48 3rd */
    rune_factory_.crossout = true;
    break;

  case 20:
    /* fraktur */
    assert(!"UNIMPLEMENTED");
    break;

  case 21:
    /* doubly-underlined, ecma 48, 3rd */
    assert(!"UNIMPLEMENTED");
    break;

  case 22:
    /* normal neither bold nor faint, ecma 48 3rd */
    rune_factory_.isBold = false;
    break;

  case 23:
    /* not italicized, ecma 48, 3rd */
    rune_factory_.isItalic = false;
    break;

  case 24:
    /* not underlined, ecma 48, 3rd */
    rune_factory_.underline = false;
    break;

  case 25:
    /* steady (not blinking), ecma 48 3rd */
    rune_factory_.blink = rune::Blink::STEADY;
    break;

  case 27:
    /* positive (not inverse), ecma 48 3rd */
    // r->invert = false;
    break;

  case 28:
    /* visible (not hidden), ecma 48 3rd, vt300 */
    assert(!"UNIMPLEMENTED");
    break;

  case 29:
    /* not crossed-out, ecma 48 3rd */
    rune_factory_.crossout = false;
    break;

  case 30 ... 37:
    /* set fg color palette */
    {
      const auto iterator = Colors.find(command);
      if (Colors.cend() != iterator) {
        rune_factory_.foregroundColor = iterator->second;
      }
    } break;

  case 38:
    /* foreground color */
    break;

  case 39:
    /* set foreground color to default, ecma 48 3rd */
    rune_factory_.resetForegroundColor();
    break;

  case 40 ... 47:
    /* set background color to default, ecma 48 3rd */
    {
      const auto iterator = Colors.find(command);
      if (Colors.cend() != iterator) {
        rune_factory_.backgroundColor = iterator->second;
      }
    } break;

  case 48:
    /* background color */
    break;

  case 49:
    /* set background color to default, ecma 48 3rd */
    rune_factory_.resetBackgroundColor();
    break;

  case 51:
    /* framed */
    assert(!"UNIMPLEMENTED");
    break;

  case 52:
    /* encircled (not widely supported) */
    assert(!"UNIMPLEMENTED");
    break;

  case 53:
    /* overlined (widely supported extension) */
    assert(!"UNIMPLEMENTED");
    break;

  case 59:
    /* set underline color to default (widely supported extension) */
    assert(!"UNIMPLEMENTED");
    break;

  case 60:
    /* ideogram */
    assert(!"UNIMPLEMENTED");
    break;

  case 73:
    /* superscript (non-standard extension) */
    assert(!"UNIMPLEMENTED");
    break;

  case 74:
    /* subscript (non-standard extension) */
    assert(!"UNIMPLEMENTED");
    break;

  case 90 ... 97:
    /* set fg color palette */
    handleSGRCommand(command - 60);
    // assert(!"UNIMPLEMENTED");
    break;

  case 100 ... 107:
    /* set background color palette */
    handleSGRCommand(command - 60);
    // assert(!"UNIMPLEMENTED");
    break;

  default:
    std::cerr << "Unknown SGR command " << command << std::endl;
    assert(!"UNKNOWN SGR CODE");
    break;
  }
}

void vt100::handleSGR() {
  std::cerr << "Full SGR escape sequence " << escapeSequence_.data() << std::endl;

  std::vector<const char *> codes;

  {
    const EscapeSequence::iterator END = escapeSequence_.end();
    EscapeSequence::iterator token = escapeSequence_.begin(),
      iterator = token;
    assert(token != END);
    for (; END != token; ++iterator) {
      if (':' == *iterator || ';' == *iterator || '\0' == *iterator) {
        *iterator = '\0';
        assert(END != token);
        codes.push_back(token.base());
        token = ++iterator;
      } else {
        if ('0' > *iterator || '9' < *iterator) {
          std::cerr << "*iterator " << *iterator << " " << std::endl;
        }
        assert('0' <= *iterator && '9' >= *iterator);
      }
    }
    assert(!codes.empty());
  }

  /* requires a color parser */
  if (0 == strcmp("38", codes[0]) || 0 == strcmp("48", codes[0])) {
    const Color color = handleSGRColor(codes);
    return;
  }

  for (const char * const code : codes) {
    const int command = std::atoi(code);
    handleSGRCommand(command);
  }
}

void vt100::handleCSI(const char c) {
  escapeSequence_.push_back(c);
  assert(/* arbitrary size */ 1024 > escapeSequence_.size());
  const bool terminated = std::isalpha(c)
    || '@' == c
    || '{' == c
    || '}' == c
    || '~' == c
    || '|' == c;

  if (terminated) {
    escapeSequence_.push_back('\0');
    assert(1 < escapeSequence_.size());
    std::cerr << "Full CSI escape sequence " << escapeSequence_.data() << std::endl;
    const char firstCharacter = escapeSequence_[0],
      lastCharacter = escapeSequence_[escapeSequence_.size() - 2],
      secondLastCharacter = escapeSequence_.size() < 3 ? '\0' :
        escapeSequence_[escapeSequence_.size() - 3];

      const bool isMultipleArguments = escapeSequence_.cend() == std::find_if(
        escapeSequence_.cbegin(),
        escapeSequence_.cend(),
        [](const char c){ return ';' == c || ':' == c; });

    switch (firstCharacter) {
    case '!':
      break;
    case '?':
      switch (secondLastCharacter) {
      case '$':
        switch (lastCharacter) {
        case 'p':
          assert(!"UNIMPLEMENTED");
          break;
        default:
          assert(!"INVALID ESCAPE SEQUENCE");
          break;
        }
        break;
      default:
        switch (lastCharacter) {
        case 'h': /* dec private mode set */
        case 'l': /* dec private mode reset */
          {
            const bool mode = 'h' == lastCharacter;
            const EscapeSequence::const_iterator END = escapeSequence_.cend();
            EscapeSequence::const_iterator token = escapeSequence_.cbegin() + 1,
              iterator = token;


            assert(token != END);
            for (; END != token; ++iterator) {
              if (':' == *iterator || ';' == *iterator || lastCharacter == *iterator) {
                assert(END != token);
                const unsigned int code = std::atoi(token.base());
                handleDecMode(code, mode);
                token = iterator++;
              } if ('\0' == *iterator) {
                assert(1 == iterator - token && lastCharacter == *token);
                break;
              } else {
                if ('0' > *iterator || '9' < *iterator) {
                  std::cerr << "*iterator " << *iterator << " " << lastCharacter << std::endl;
                }
                assert('0' <= *iterator && '9' >= *iterator);
              }
            }
          }
          break;
        case 'i':
          /* media copy */
          assert(!"UNIMPLEMENTED");
          break;
        case 'S':
          /* set or request graphics attribute */
          assert(!"UNIMPLEMENTED");
          break;
        case 'n':
          /* device status report */
          assert(!"UNIMPLEMENTED");
          break;
        default:
          assert(!"INVALID ESCAPE SEQUENCE");
          break;
        }
      }
      break;
    case '>':
      break;
    case '=':
      switch (lastCharacter) {
      case 'c':
        assert(!"UNIMPLEMENTED");
        break;
      default:
        assert(!"INVALID ESCAPE SEQUENCE");
        break;
      }
      break;
    default:
      switch (lastCharacter) {
      case 'm':
        /* SGR - change one of more text attributes */
        assert('\0' == escapeSequence_.back());
        escapeSequence_.pop_back();
        assert('m' == escapeSequence_.back());
        escapeSequence_.pop_back();
        escapeSequence_.push_back('\0');
        handleSGR();
        break;

      case '@':
        /* The normal character attribute.
         * The cursor remains at the beginning of the blank characters.
         * Text between the cursor and right margin moves to the right.
         */
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.insert(argument);
        }
        break;

      case 'a':
        /* HPR - move cursor right (forward) Ps lines */
        assert(!"UNIMPLEMENTED");
        break;

      case 'A':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor_up(0 < argument ? argument : 1);
        }
        break;

      case 'B':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor_down(0 < argument ? argument : 1);
        }
        break;

      case 'C':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor_forward(0 < argument ? argument : 1);
        }
        break;

      case 'D':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor_backward(0 < argument ? argument : 1);
        }
        break;

      case 'E':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor(1, screen_.line() + 0 < argument ? argument : 1);
        }
        break;

      case 'F':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor(1, screen_.line() - 0 < argument ? argument : 1);
        }
        break;

      case 'G':
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.move_cursor(0 < argument ? argument : 1, screen_.line());
        }
        break;

      case 'H':
        {
          uint16_t column = 1, line = 1;
          if (2 < escapeSequence_.size()) {
            std::istringstream stream{
              std::string{escapeSequence_.begin(), escapeSequence_.end()}};
            stream >> column;
            char delimiter;
            stream >> delimiter;
            assert(';' == delimiter);
            stream >> line;
            column = std::max<uint16_t>(1, column);
            line = std::max<uint16_t>(1, line);
            assert(screen_.lines() >= line);
            assert(screen_.columns() >= column);
          }
          screen_.move_cursor(column, line);
        } break;

      case 'J':
        {
          const std::size_t size = escapeSequence_.size();
          if (2 == size) {
            screen_.erase_display();
          } else if (3 == size)  {
            const int32_t argument = std::atoi(escapeSequence_.data());
            switch (argument) {
            case 2:
              screen_.erase_display();
              break;
            case 3:
              screen_.erase_scrollback();
              break;
            default:
              assert(!"UNIMPLEMENTED");
              break;
            }
          }
        } break;

      case 'K':
        screen_.erase_line_right();
        break;

      case 'L':
        /* IL - insert line at cursor shift rest down */
        assert(!"UNIMPLEMENTED");
        break;

      case 'e':
        /* VPR - move cursor down Ps lines */
        assert(!"UNIMPLEMENTED");
        break;

      case '`':
        /* CBT - move cursor to column Ps */
        assert(!"UNIMPLEMENTED");
        break;

      case 'd':
        /* VPA - move cursor to row Ps */
        assert(!"UNIMPLEMENTED");
        break;

      case 'r': {
          /* DECSTBM - set scroll region */
          //TODO: make a function
          int64_t top = 1, bottom = 1;
          if ('r' != escapeSequence_.front()) {
            std::istringstream stream{
              std::string{escapeSequence_.begin(), escapeSequence_.end()}};
            stream >> top;
            char delimiter;
            stream >> delimiter;
            assert(';' == delimiter);
            stream >> bottom;
            top = std::max<int64_t>(1, top);
            bottom = 1 > bottom ? screen_.lines() - 1 : bottom;
            --top;
            --bottom;
            std::cout << top << ", " << bottom << std::endl;
          } else {
            assert(!"unimplemented");
          }

          if (bottom > top) {
            screen_.move_cursor(1, 1);
          } else {
            assert(!"invalid DECSTBM sequence");
          }
        } break;

      case 'I':
        /* CHT - cursor forward Ps tabulations */
        assert(!"UNIMPLEMENTED");
        break;

      case 'Z':
        /* CBT - cursor back Ps tabulations */
        assert(!"UNIMPLEMENTED");
        break;

      case 'h':
        /* SM - set mode */
        assert(!"UNIMPLEMENTED");
        break;

      case 'l':
        /* RM - reset mode */
        assert(!"UNIMPLEMENTED");
        break;

      case 'g':
        /* TBC - tabulation clear */
        assert(!"UNIMPLEMENTED");
        break;

      case 'f':
        /* HVP - move cursor to Px, Py */
        assert(!"UNIMPLEMENTED");
        break;

      case 'c':
        /* report vt340 type device with sixel */
        assert(!"UNIMPLEMENTED");
        break;

      case 'n':
        /* DSR - device status report */
        {
          const int argument = std::atoi(escapeSequence_.data());
          reportDeviceStatus(argument);
        } break;

      case 'M':
        /* DL - delete lines */
        assert(!"UNIMPLEMENTED");
        break;

      case 'T':
        /* SD - scroll down */
        assert(!"UNIMPLEMENTED");
        break;

      case 'X':
        /* ECH - erase Ps characters */
        assert(!"UNIMPLEMENTED");
        break;

      case 'P':
        /* DCH - erase Ps characters */
        {
          const int argument = std::atoi(escapeSequence_.data());
          screen_.erase(argument);
        }
        break;

      case 'i':
        assert(!"UNIMPLEMENTED");
        break;

      case 'u':
        /* SCORC - restore cursor */
        /* DECSMBV - set margin-bell volume */
        assert(!"UNIMPLEMENTED");
        break;

      case 's':
        /* DECSLRM - set left and right margin */
        assert(!"UNIMPLEMENTED");
        break;

      case 'q':
        /* DECLL - manipulate keyboard leds */
        assert(!"UNIMPLEMENTED");
        break;

      case 't':
        /* XTWINOPS - xterm window ops */
        assert(!"UNIMPLEMENTED");
        break;

      default:
        assert(!"INVALID ESCAPE SEQUENCE");
        break;
      }
      break;
    }
    escapeSequence_.clear();
    state_ = LITERAL;
  }
}

void vt100::handleOSC(const char c) {
  assert(/* arbitrary size */ 1024 > escapeSequence_.size());
  const bool terminated = '\a' == c
    || (1 <= escapeSequence_.size()
    && '\e' == escapeSequence_.back()
    && '\\' == c);

  escapeSequence_.push_back(c);

  if (terminated) {
    if ('\a' == escapeSequence_.back()) {
      escapeSequence_.pop_back();
    } else if ('\\' == escapeSequence_.back()) {
      escapeSequence_.pop_back();
      if ('\e' == escapeSequence_.back()) {
        escapeSequence_.pop_back();
      } else {
        assert(!"UNREACHEABLE");
      }
    } else {
      assert(!"UNREACHEABLE");
    }

    escapeSequence_.push_back('\0');

    assert(1 < escapeSequence_.size());

    const unsigned int code = std::atoi(escapeSequence_.data());
    switch (code) {
    case 0:
      /* change window title */
      {
        const std::string title = escapeSequence_.data() + 2;
        screen_.setTitle(title);
      } break;

    default:
      assert(!"UNIMPLEMENTED");
      break;
    }

    escapeSequence_.clear();
    state_ = LITERAL;
  }
}

void vt100::handleDCS(const char) { }

void vt100::handlePM(const char) { }

vt100::CharacterType vt100::handleCharacter(const wchar_t c) {
  switch(state_) {
  case LITERAL:
    switch (c) {
    case '\e':
      state_ = ESCAPE;
      break;
    case '\n':
      screen_.pushBack(rune_factory_.make(c));
      return CharacterType::terminal;
    default:
      screen_.pushBack(rune_factory_.make(c));
      break;
    }
    break;

  case CSI:
    switch (c) {
    case '\a':
      /* bell */
      break;
    case '\b':
      /* backspace */
      break;
    case '\r':
      /* carriage return */
      break;
    case '\f':
    case '\v':
    case '\n':
      /* carriage return */
      /* line feed */
      break;
    default:
      handleCSI(c);
      break;
    }
    break;

  case ESCAPE:
    std::cerr << "Escape sequence " << static_cast<char>(c) << std::endl;
    switch (c) {
    case '[':
      state_ = CSI;
      break;
    case ']':
      state_ = OSC;
      break;
    case 'P':
      state_ = DCS;
      break;
    case '_':
      state_ = APC;
      break;
    case 'M':
      /* reverse line feed */
      screen_.reverse_line_feed();
      state_ = LITERAL;
      break;
    case 'E':
      /* new line */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'D':
      /* line feed */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '#':
      assert(!"UNIMPLEMENTED");
      state_ = DEC;
      break;
    case 'H':
      /* set tab stop at current column */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '(':
      state_ = CHARSET_G0;
      break;
    case ')':
      state_ = CHARSET_G1;
      break;
    case '*':
      state_ = CHARSET_G2;
      break;
    case '+':
      state_ = CHARSET_G3;
      break;
    case '%':
      state_ = OTHER_ESC_CTL_SEQ;
      break;
    case ' ':
      state_ = OTHER_ESC_CTL_SEQ;
      break;
    case 'g':
      /* bell */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '=':
      /* application keypad */
      std::cerr << "set application keypad mode" << std::endl;
      state_ = LITERAL;
      break;
    case '>':
      /* normal keypad */
      std::cerr << "set normal keypad mode" << std::endl;
      state_ = LITERAL;
      break;
    case '`':
      /* disable manual input */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'b':
      /* enable manual input */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'c':
      /* reset initial state */
      std::cerr << "reset initial state" << std::endl;
      rune_factory_.reset();
      state_ = LITERAL;
      break;
    case '7':
      /* save cursor */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '8':
      /* restore cursor */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '6':
      /* DECBI */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '9':
      /* DECFI */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'd':
      /* coding method delimiter */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'n':
      /* invoke the g2 charset into gl */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'o':
      /* invoke the g2 charset into gl */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '|':
      /* invoke the g3 charset into gl */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '}':
      /* invoke the g2 charset into gl */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case '~':
      /* invoke the g1 charset into gl */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'N':
      /* single shift select of g2 character set */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'O':
      /* single shift select of g3 character set */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'k':
      /* old tabtitle set sequence */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'X':
      /* start of string */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'V':
      /* start of guarded area */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;
    case 'W':
      /* end of guarded area */
      assert(!"UNIMPLEMENTED");
      state_ = LITERAL;
      break;

    case '\\':
      /* st */
      return CharacterType::nonterminal;

    case '\e':
      return CharacterType::nonterminal;

    default:
      assert(!"INVALID ESCAPE SEQUENCE");
      state_ = LITERAL;
      break;
    }
    break;
  case CHARSET_G0:
    assert(!"UNIMPLEMENTED");
    break;
  case CHARSET_G1:
    assert(!"UNIMPLEMENTED");
    break;
  case CHARSET_G2:
    assert(!"UNIMPLEMENTED");
    break;
  case CHARSET_G3:
    assert(!"UNIMPLEMENTED");
    break;
  case OTHER_ESC_CTL_SEQ:
    assert(!"UNIMPLEMENTED");
    state_ = LITERAL;
    break;
  case OSC:
    handleOSC(c);
    break;
  case PM:
    handlePM(c);
    break;
  case DCS:
    handleDCS(c);
    break;
  case APC:
    handleAPC(c);
    break;
  case DEC:
    assert(!"UNIMPLEMENTED");
    break;
  case TITLE:
    assert(!"UNIMPLEMENTED");
    break;
  default:
    assert(!"UNREACHABLE");
    break;
  }
  return CharacterType::terminal;
}

void vt100::reportDeviceStatus(const int argument) {
  switch (argument) {
  case 6: /* report cursor position */
    escape("\e[?%u;%uR", screen_.line(), screen_.column());
    break;
  default:
    assert(!"UNIMPLEMENTED");
    break;
  }
}

Color vt100::handleSGRColor(const std::vector<const char *> & codes) {
  for (auto component : codes) {
    std::cout << component << ", ";
  }
  std::cout << std::endl;
  return colors::white;
}

void vt100::handleEscape(const char * const sequence, const int length) {
  for (int i = 0; length > i; ++i) {
    handleCharacter(sequence[i]);
  }
}

bool vt100::pollin(const std::optional<TimePoint> & t) {
  while (true) {
    while (bufferIndex_ < bufferSize_) {
      wchar_t character = 0;
      {
        const ssize_t size = bufferSize_ - bufferIndex_;
#if 1
        const ssize_t bytes = mbrtowc(&character, &buffer_[bufferIndex_], size, /* mbstate_t = */ nullptr);
#else
        const ssize_t bytes = 1;
        character = static_cast<wchar_t>(buffer_[bufferIndex_]);
#endif
        assert(4 >= bytes);
        if (bytes > size) {
          std::cerr << __func__ << " " << bytes << " " << size << std::endl;
        }
        assert(bytes <= size);
        switch (bytes) {
        case -2: /* incomplete */
          if (0 < bufferIndex_) {
            strncpy(&buffer_[0], &buffer_[bufferIndex_], size);
          } else {
            /* this means there is an incomplete sequence right at the beginning of the buffer */
            std::cerr << "also odd" << std::endl;
          }
          bufferStart_ = size;
          assert(4 > bufferStart_);
          bufferIndex_ += size;
          assert(bufferIndex_ == bufferSize_);
          continue;
          break;

        case -1: /* invalid */
          std::cerr << "problems with utf-8: invalid number of bytes" << std::endl;
          assert(!"invalid");
          break;

        case 0:
          std::cerr << "odd" << std::endl;
          assert(0 == character);
          break;

        default:
          assert(0 < character);
          bufferIndex_ += bytes;
          const CharacterType type = handleCharacter(character);
          /* if character type is terminal, we may return control */
          if (CharacterType::terminal == type) {
            if (t.has_value() && t.value() <= std::chrono::steady_clock::now()) {
              return false;
            }
          }
          break;
        }
      }
    }

    assert(bufferSize_ >= bufferIndex_);
    
    {
      assert(4 > bufferStart_);
      const ssize_t size = read(fd_.child, buffer_.data() + bufferStart_, buffer_.size() - bufferStart_);
      if (0 > size) {
        if (EAGAIN == errno) {
          break; // nothing left to read.
        } else if (EIO == errno) {
          std::cerr << "A signal has not been properly caught " << errno << " " << strerror(errno) << std::endl;
          break;
        } else {
          std::cerr << "errno " << errno << " " << strerror(errno) << std::endl;
          assert(!"ERROR");
        }
      } else {
        bufferSize_ = bufferStart_ + size;
        assert(buffer_.size() >= bufferSize_);
        if (0 == size) {
          break; // complete read operation.
        } else {
          bufferStart_ = bufferIndex_ = 0;
        }
      }
    }
  }
  return true;
}

void vt100::alternative_buffer_off() {
  screen_.alternative(false);
  rune_factory_.reset();
}

void vt100::alternative_buffer_on() {
  screen_.alternative(true);
}

