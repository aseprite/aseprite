// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/entry.h"

#include "ui/clipboard.h"
#include "ui/font.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdarg.h>
#include <stdio.h>

#define CHARACTER_LENGTH(f, c) ((f)->vtable->char_length((f), (c)))

namespace ui {

Entry::Entry(size_t maxsize, const char *format, ...)
  : Widget(kEntryWidget)
  , m_timer(500, this)
{
  char buf[4096];

  // formatted string
  if (format) {
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);
  }
  // empty string
  else {
    ustrcpy(buf, empty_string);
  }

  m_maxsize = maxsize;
  m_caret = 0;
  m_scroll = 0;
  m_select = 0;
  m_hidden = false;
  m_state = false;
  m_password = false;
  m_readonly = false;
  m_recent_focused = false;

  /* TODO support for text alignment and multi-line */
  /* widget->align = JI_LEFT | JI_MIDDLE; */
  setText(buf);

  this->setFocusStop(true);
  initTheme();
}

Entry::~Entry()
{
}

bool Entry::isReadOnly() const
{
  return m_readonly;
}

bool Entry::isPassword() const
{
  return m_password;
}

void Entry::setReadOnly(bool state)
{
  m_readonly = state;
}

void Entry::setPassword(bool state)
{
  m_password = state;
}

void Entry::showCaret()
{
  m_hidden = false;
  invalidate();
}

void Entry::hideCaret()
{
  m_hidden = true;
  invalidate();
}

void Entry::setCaretPos(int pos)
{
  const char *text = this->getText();
  int x, c;

  m_caret = pos;

  /* backward scroll */
  if (m_caret < m_scroll)
    m_scroll = m_caret;

  /* forward scroll */
  m_scroll--;
  do {
    x = this->rc->x1 + this->border_width.l;
    for (c=++m_scroll; ; c++) {
      x += CHARACTER_LENGTH(this->getFont(),
                            (c < ustrlen(text))? ugetat(text, c): ' ');

      if (x >= this->rc->x2-this->border_width.r)
        break;
    }
  } while (m_caret >= c);

  m_timer.start();
  m_state = true;

  invalidate();
}

void Entry::selectText(int from, int to)
{
  int end = ustrlen(this->getText());

  m_select = from;
  setCaretPos(from); // to move scroll
  setCaretPos((to >= 0)? to: end+to+1);

  invalidate();
}

void Entry::deselectText()
{
  m_select = -1;
  invalidate();
}

void Entry::setSuffix(const std::string& suffix)
{
  m_suffix = suffix;
  invalidate();
}

void Entry::getEntryThemeInfo(int* scroll, int* caret, int* state,
                              int* selbeg, int* selend)
{
  if (scroll) *scroll = m_scroll;
  if (caret) *caret = m_caret;
  if (state) *state = !m_hidden && m_state;

  if ((m_select >= 0) &&
      (m_caret != m_select)) {
    *selbeg = MIN(m_caret, m_select);
    *selend = MAX(m_caret, m_select)-1;
  }
  else {
    *selbeg = -1;
    *selend = -1;
  }
}

bool Entry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kTimerMessage:
      if (hasFocus() && static_cast<TimerMessage*>(msg)->timer() == &m_timer) {
        // Blinking caret
        m_state = m_state ? false: true;
        invalidate();
      }
      break;

    case kFocusEnterMessage:
      m_timer.start();

      m_state = true;
      invalidate();

      selectText(0, -1);
      m_recent_focused = true;
      break;

    case kFocusLeaveMessage:
      invalidate();

      m_timer.stop();

      deselectText();
      m_recent_focused = false;
      break;

    case kKeyDownMessage:
      if (hasFocus() && !isReadOnly()) {
        // Command to execute
        EntryCmd::Type cmd = EntryCmd::NoOp;
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        switch (scancode) {

          case KEY_LEFT:
            if (msg->ctrlPressed())
              cmd = EntryCmd::BackwardWord;
            else
              cmd = EntryCmd::BackwardChar;
            break;

          case KEY_RIGHT:
            if (msg->ctrlPressed())
              cmd = EntryCmd::ForwardWord;
            else
              cmd = EntryCmd::ForwardChar;
            break;

          case KEY_HOME:
            cmd = EntryCmd::BeginningOfLine;
            break;

          case KEY_END:
            cmd = EntryCmd::EndOfLine;
            break;

          case KEY_DEL:
            if (msg->shiftPressed())
              cmd = EntryCmd::Cut;
            else
              cmd = EntryCmd::DeleteForward;
            break;

          case KEY_INSERT:
            if (msg->shiftPressed())
              cmd = EntryCmd::Paste;
            else if (msg->ctrlPressed())
              cmd = EntryCmd::Copy;
            break;

          case KEY_BACKSPACE:
            cmd = EntryCmd::DeleteBackward;
            break;

          default:
            if (keymsg->ascii() >= 32) {
              // Ctrl and Alt must be unpressed to insert a character
              // in the text-field.
              if ((msg->keyModifiers() & (kKeyCtrlModifier | kKeyAltModifier)) == 0) {
                cmd = EntryCmd::InsertChar;
              }
            }
            else {
              // Map common Windows shortcuts for Cut/Copy/Paste
              if (msg->onlyCtrlPressed()) {
                switch (scancode) {
                  case kKeyX: cmd = EntryCmd::Cut; break;
                  case kKeyC: cmd = EntryCmd::Copy; break;
                  case kKeyV: cmd = EntryCmd::Paste; break;
                }
              }
            }
            break;
        }

        if (cmd == EntryCmd::NoOp)
          break;

        executeCmd(cmd, keymsg->ascii(),
                   (msg->shiftPressed()) ? true: false);
        return true;
      }
      break;

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        const char *text = this->getText();
        int c, x;

        bool move = true;
        bool is_dirty = false;

        // Backward scroll
        if (mousePos.x < this->rc->x1) {
          if (m_scroll > 0) {
            m_caret = --m_scroll;
            move = false;
            is_dirty = true;
            invalidate();
          }
        }
        // Forward scroll
        else if (mousePos.x >= this->rc->x2) {
          if (m_scroll < ustrlen(text)) {
            m_scroll++;
            x = this->rc->x1 + this->border_width.l;
            for (c=m_scroll; ; c++) {
              x += CHARACTER_LENGTH(this->getFont(),
                                   (c < ustrlen(text))? ugetat(text, c): ' ');
              if (x > this->rc->x2-this->border_width.r) {
                c--;
                break;
              }
              else if (!ugetat (text, c))
                break;
            }
            m_caret = c;
            move = false;
            is_dirty = true;
            invalidate();
          }
        }

        // Move caret
        if (move) {
          c = getCaretFromMouse(static_cast<MouseMessage*>(msg));

          if (m_caret != c) {
            m_caret = c;
            is_dirty = true;
            invalidate();
          }
        }

        // Move selection
        if (m_recent_focused) {
          m_recent_focused = false;
          m_select = m_caret;
        }
        else if (msg->type() == kMouseDownMessage)
          m_select = m_caret;

        // Show the caret
        if (is_dirty) {
          m_timer.start();
          m_state = true;
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture())
        releaseMouse();
      return true;

    case kDoubleClickMessage:
      forwardWord();
      m_select = m_caret;
      backwardWord();
      invalidate();
      return true;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      /* TODO theme stuff */
      if (isEnabled())
        invalidate();
      break;
  }

  return Widget::onProcessMessage(msg);
}

void Entry::onPreferredSize(PreferredSizeEvent& ev)
{
  int w =
    + border_width.l
    + ji_font_char_len(getFont(), 'w') * MIN(m_maxsize, 6)
    + 2 + border_width.r;

  w = MIN(w, JI_SCREEN_W/2);

  int h =
    + border_width.t
    + text_height(getFont())
    + border_width.b;

  ev.setPreferredSize(w, h);
}

void Entry::onPaint(PaintEvent& ev)
{
  getTheme()->paintEntry(ev);
}

void Entry::onSetText()
{
  Widget::onSetText();

  if (m_caret >= 0 && (size_t)m_caret > getTextSize())
    m_caret = (int)getTextSize();
}

void Entry::onEntryChange()
{
  EntryChange();
}

int Entry::getCaretFromMouse(MouseMessage* mousemsg)
{
  int c, x, w, mx, caret = m_caret;

  mx = mousemsg->position().x;
  mx = MID(this->rc->x1+this->border_width.l,
           mx,
           this->rc->x2-this->border_width.r-1);

  x = this->rc->x1 + this->border_width.l;
  for (c=m_scroll; ugetat(this->getText(), c); c++) {
    w = CHARACTER_LENGTH(this->getFont(), ugetat(this->getText(), c));
    if (x+w >= this->rc->x2-this->border_width.r)
      break;
    if ((mx >= x) && (mx < x+w)) {
      caret = c;
      break;
    }
    x += w;
  }

  if (!ugetat(this->getText(), c))
    if ((mx >= x) &&
        (mx <= this->rc->x2-this->border_width.r-1))
      caret = c;

  return caret;
}

void Entry::executeCmd(EntryCmd::Type cmd, int ascii, bool shift_pressed)
{
  std::string text = getText();
  int c, selbeg, selend;

  getEntryThemeInfo(NULL, NULL, NULL, &selbeg, &selend);

  switch (cmd) {

    case EntryCmd::NoOp:
      break;

    case EntryCmd::InsertChar:
      // delete the entire selection
      if (selbeg >= 0) {
        text.erase(selbeg, selend-selbeg+1);

        m_caret = selbeg;
      }

      // put the character
      if (text.size() < m_maxsize) {
        ASSERT((size_t)m_caret <= text.size());
        text.insert(m_caret++, 1, ascii);
      }

      m_select = -1;
      break;

    case EntryCmd::BackwardChar:
    case EntryCmd::BackwardWord:
      // selection
      if (shift_pressed) {
        if (m_select < 0)
          m_select = m_caret;
      }
      else
        m_select = -1;

      // backward word
      if (cmd == EntryCmd::BackwardWord) {
        backwardWord();
      }
      // backward char
      else if (m_caret > 0) {
        m_caret--;
      }
      break;

    case EntryCmd::ForwardChar:
    case EntryCmd::ForwardWord:
      // selection
      if (shift_pressed) {
        if (m_select < 0)
          m_select = m_caret;
      }
      else
        m_select = -1;

      // forward word
      if (cmd == EntryCmd::ForwardWord) {
        forwardWord();
      }
      // forward char
      else if (m_caret < (int)text.size()) {
        m_caret++;
      }
      break;

    case EntryCmd::BeginningOfLine:
      // selection
      if (shift_pressed) {
        if (m_select < 0)
          m_select = m_caret;
      }
      else
        m_select = -1;

      m_caret = 0;
      break;

    case EntryCmd::EndOfLine:
      // selection
      if (shift_pressed) {
        if (m_select < 0)
          m_select = m_caret;
      }
      else
        m_select = -1;

      m_caret = text.size();
      break;

    case EntryCmd::DeleteForward:
    case EntryCmd::Cut:
      // delete the entire selection
      if (selbeg >= 0) {
        // *cut* text!
        if (cmd == EntryCmd::Cut) {
          base::string buf = text.substr(selbeg, selend - selbeg + 1);
          clipboard::set_text(buf.c_str());
        }

        // remove text
        text.erase(selbeg, selend-selbeg+1);

        m_caret = selbeg;
      }
      // delete the next character
      else {
        if (m_caret < (int)text.size())
          text.erase(m_caret, 1);
      }

      m_select = -1;
      break;

    case EntryCmd::Paste: {
      const char *clipboard;

      if ((clipboard = clipboard::get_text())) {
        // delete the entire selection
        if (selbeg >= 0) {
          text.erase(selbeg, selend-selbeg+1);

          m_caret = selbeg;
          m_select = -1;
        }

        // paste text
        for (c=0; c<ustrlen(clipboard); c++)
          if (text.size() < m_maxsize)
            text.insert(m_caret+c, 1, ugetat(clipboard, c));
          else
            break;

        setCaretPos(m_caret+c);
      }
      break;
    }

    case EntryCmd::Copy:
      if (selbeg >= 0) {
        base::string buf = text.substr(selbeg, selend - selbeg + 1);
        clipboard::set_text(buf.c_str());
      }
      break;

    case EntryCmd::DeleteBackward:
      // delete the entire selection
      if (selbeg >= 0) {
        text.erase(selbeg, selend-selbeg+1);

        m_caret = selbeg;
      }
      // delete the previous character
      else {
        if (m_caret > 0)
          text.erase(--m_caret, 1);
      }

      m_select = -1;
      break;
  }

  if (text != this->getText()) {
    this->setText(text.c_str());
    onEntryChange();
  }

  setCaretPos(m_caret);
  invalidate();
}

#define IS_WORD_CHAR(ch)                                \
  (!((!ch) || (uisspace(ch)) ||                         \
    ((ch) == '/') || ((ch) == OTHER_PATH_SEPARATOR)))

void Entry::forwardWord()
{
  int ch;

  for (; m_caret<ustrlen(this->getText()); m_caret++) {
    ch = ugetat(this->getText(), m_caret);
    if (IS_WORD_CHAR (ch))
      break;
  }

  for (; m_caret<ustrlen(this->getText()); m_caret++) {
    ch = ugetat(this->getText(), m_caret);
    if (!IS_WORD_CHAR(ch)) {
      m_caret++;
      break;
    }
  }
}

void Entry::backwardWord()
{
  int ch;

  for (m_caret--; m_caret >= 0; m_caret--) {
    ch = ugetat(this->getText(), m_caret);
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; m_caret >= 0; m_caret--) {
    ch = ugetat(this->getText(), m_caret);
    if (!IS_WORD_CHAR(ch)) {
      m_caret++;
      break;
    }
  }

  if (m_caret < 0)
    m_caret = 0;
}

} // namespace ui
