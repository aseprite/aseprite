// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/entry.h"

#include "base/bind.h"
#include "base/string.h"
#include "clip/clip.h"
#include "she/font.h"
#include "ui/manager.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>

namespace ui {

Entry::Entry(std::size_t maxsize, const char *format, ...)
  : Widget(kEntryWidget)
  , m_timer(500, this)
  , m_maxsize(maxsize)
  , m_caret(0)
  , m_scroll(0)
  , m_select(0)
  , m_hidden(false)
  , m_state(false)
  , m_readonly(false)
  , m_password(false)
  , m_recent_focused(false)
  , m_lock_selection(false)
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
    buf[0] = 0;
  }

  // TODO support for text alignment and multi-line
  // widget->align = LEFT | MIDDLE;
  setText(buf);

  setFocusStop(true);
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
  auto utf8_begin = base::utf8_const_iterator(text().begin());
  auto utf8_end = base::utf8_const_iterator(text().end());
  int textlen = base::utf8_length(text());

  m_caret = MID(0, pos, textlen);

  // Backward scroll
  if (m_scroll > m_caret)
    m_scroll = m_caret;

  // Forward scroll
  --m_scroll;
  int c;
  while (true) {
    c = ++m_scroll;
    auto utf8_it = utf8_begin + MID(0, c, textlen);
    int x = bounds().x + border().left()
      - font()->charWidth(' '); // Space for the carret
    for (; utf8_it != utf8_end; ++c, ++utf8_it) {
      int ch = *utf8_it;
      x += font()->charWidth(ch);
      if (x >= bounds().x2()-border().right())
        break;
    }
    if (m_caret < c || utf8_it == utf8_end)
      break;
  }

  m_timer.start();
  m_state = true;

  invalidate();
}

void Entry::selectText(int from, int to)
{
  int end = base::utf8_length(text());

  m_select = from;
  setCaretPos(from); // to move scroll
  setCaretPos((to >= 0)? to: end+to+1);

  invalidate();
}

void Entry::selectAllText()
{
  selectText(0, -1);
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

gfx::Rect Entry::getEntryTextBounds() const
{
  return onGetEntryTextBounds();
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

      if (m_lock_selection) {
        m_lock_selection = false;
      }
      else {
        selectAllText();
        m_recent_focused = true;
      }
      break;

    case kFocusLeaveMessage:
      invalidate();

      m_timer.stop();

      if (!m_lock_selection)
        deselectText();

      m_recent_focused = false;
      break;

    case kKeyDownMessage:
      if (hasFocus() && !isReadOnly()) {
        // Command to execute
        EntryCmd cmd = EntryCmd::NoOp;
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        switch (scancode) {

          case kKeyLeft:
            if (msg->ctrlPressed() || msg->altPressed())
              cmd = EntryCmd::BackwardWord;
            else if (msg->cmdPressed())
              cmd = EntryCmd::BeginningOfLine;
            else
              cmd = EntryCmd::BackwardChar;
            break;

          case kKeyRight:
            if (msg->ctrlPressed() || msg->altPressed())
              cmd = EntryCmd::ForwardWord;
            else if (msg->cmdPressed())
              cmd = EntryCmd::EndOfLine;
            else
              cmd = EntryCmd::ForwardChar;
            break;

          case kKeyHome:
            cmd = EntryCmd::BeginningOfLine;
            break;

          case kKeyEnd:
            cmd = EntryCmd::EndOfLine;
            break;

          case kKeyDel:
            if (msg->shiftPressed())
              cmd = EntryCmd::Cut;
            else if (msg->ctrlPressed())
              cmd = EntryCmd::DeleteForwardToEndOfLine;
            else
              cmd = EntryCmd::DeleteForward;
            break;

          case kKeyInsert:
            if (msg->shiftPressed())
              cmd = EntryCmd::Paste;
            else if (msg->ctrlPressed())
              cmd = EntryCmd::Copy;
            break;

          case kKeyBackspace:
            if (msg->ctrlPressed())
              cmd = EntryCmd::DeleteBackwardWord;
            else
              cmd = EntryCmd::DeleteBackward;
            break;

          default:
            // Map common Windows shortcuts for Cut/Copy/Paste
#if defined __APPLE__
            if (msg->onlyCmdPressed())
#else
            if (msg->onlyCtrlPressed())
#endif
            {
              switch (scancode) {
                case kKeyX: cmd = EntryCmd::Cut; break;
                case kKeyC: cmd = EntryCmd::Copy; break;
                case kKeyV: cmd = EntryCmd::Paste; break;
                case kKeyA: cmd = EntryCmd::SelectAll; break;
              }
            }
            else if (manager()->isFocusMovementKey(msg)) {
              break;
            }
            else if (keymsg->unicodeChar() >= 32) {
              // Ctrl and Alt must be unpressed to insert a character
              // in the text-field.
              if ((msg->modifiers() & (kKeyCtrlModifier | kKeyAltModifier)) == 0) {
                cmd = EntryCmd::InsertChar;
              }
            }
            break;
        }

        if (cmd == EntryCmd::NoOp)
          break;

        executeCmd(cmd, keymsg->unicodeChar(),
                   (msg->shiftPressed()) ? true: false);
        return true;
      }
      break;

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        auto utf8_begin = base::utf8_const_iterator(text().begin());
        auto utf8_end = base::utf8_const_iterator(text().end());
        int textlen = base::utf8_length(text());
        int c, x;

        bool move = true;
        bool is_dirty = false;

        // Backward scroll
        if (mousePos.x < bounds().x) {
          if (m_scroll > 0) {
            m_caret = --m_scroll;
            move = false;
            is_dirty = true;
            invalidate();
          }
        }
        // Forward scroll
        else if (mousePos.x >= bounds().x2()) {
          if (m_scroll < textlen - getAvailableTextLength()) {
            ++m_scroll;
            x = bounds().x + border().left();

            auto utf8_it = utf8_begin + MID(0, m_scroll, textlen);
            for (c=m_scroll; utf8_it != utf8_end; ++c, ++utf8_it) {
              int ch = (c < textlen ? *utf8_it: ' ');

              x += font()->charWidth(ch);
              if (x > bounds().x2()-border().right()) {
                c--;
                break;
              }
            }
            m_caret = MID(0, c, textlen);
            move = false;
            is_dirty = true;
            invalidate();
          }
        }

        c = getCaretFromMouse(static_cast<MouseMessage*>(msg));

        if (static_cast<MouseMessage*>(msg)->left() ||
            (move && !isPosInSelection(c))) {
          // Move caret
          if (move) {
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
        }

        // Show the caret
        if (is_dirty) {
          m_timer.start();
          m_state = true;
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        if (mouseMsg->right()) {
          // This flag is disabled in kFocusEnterMessage message handler.
          m_lock_selection = true;

          showEditPopupMenu(mouseMsg->position());
          requestFocus();
        }
      }
      return true;

    case kDoubleClickMessage:
      forwardWord();
      m_select = m_caret;
      backwardWord();
      invalidate();
      return true;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;
  }

  return Widget::onProcessMessage(msg);
}

void Entry::onSizeHint(SizeHintEvent& ev)
{
  int w =
    + font()->charWidth('w') * MIN(m_maxsize, 6)
    + 2*guiscale()
    + border().width();

  w = MIN(w, ui::display_w()/2);

  int h =
    + font()->height()
    + border().height();

  ev.setSizeHint(w, h);
}

void Entry::onPaint(PaintEvent& ev)
{
  theme()->paintEntry(ev);
}

void Entry::onSetText()
{
  Widget::onSetText();

  int textlen = textLength();
  if (m_caret >= 0 && m_caret > textlen)
    m_caret = textlen;
}

void Entry::onChange()
{
  Change();
}

gfx::Rect Entry::onGetEntryTextBounds() const
{
  gfx::Rect bounds = clientBounds();
  bounds.x += border().left();
  bounds.y += bounds.h/2 - textHeight()/2;
  bounds.w -= border().width();
  bounds.h = textHeight();
  return bounds;
}

int Entry::getCaretFromMouse(MouseMessage* mousemsg)
{
  base::utf8_const_iterator utf8_begin = base::utf8_const_iterator(text().begin());
  base::utf8_const_iterator utf8_end = base::utf8_const_iterator(text().end());
  int caret = m_caret;
  int textlen = base::utf8_length(text());
  gfx::Rect bounds = getEntryTextBounds().offset(this->bounds().origin());

  int mx = mousemsg->position().x;
  mx = MID(bounds.x, mx, bounds.x2()-1);

  int x = bounds.x;

  auto utf8_it = utf8_begin + MID(0, m_scroll, textlen);
  int c = m_scroll;
  for (; utf8_it != utf8_end; ++c, ++utf8_it) {
    int w = font()->charWidth(*utf8_it);
    if (x+w >= bounds.x2()-border().right())
      break;
    if ((mx >= x) && (mx < x+w)) {
      caret = c;
      break;
    }
    x += w;
  }

  if (utf8_it == utf8_end) {
    if ((mx >= x) && (mx < bounds.x2())) {
      caret = c;
    }
  }

  return MID(0, caret, textlen);
}

void Entry::executeCmd(EntryCmd cmd, int unicodeChar, bool shift_pressed)
{
  std::wstring text = base::from_utf8(this->text());
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
        ASSERT((std::size_t)m_caret <= text.size());
        text.insert(m_caret++, 1, unicodeChar);
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
          std::wstring selected = text.substr(selbeg, selend - selbeg + 1);
          clip::set_text(base::to_utf8(selected));
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
      std::string clipboard;
      if (clip::get_text(clipboard)) {
        // delete the entire selection
        if (selbeg >= 0) {
          text.erase(selbeg, selend-selbeg+1);

          m_caret = selbeg;
          m_select = -1;
        }

        // paste text
        for (c=0; c<base::utf8_length(clipboard); c++) {
          if (text.size() < m_maxsize)
            text.insert(m_caret+c, 1,
                        *(base::utf8_const_iterator(clipboard.begin())+c));
          else
            break;
        }

        setCaretPos(m_caret+c);
      }
      break;
    }

    case EntryCmd::Copy:
      if (selbeg >= 0) {
        std::wstring selected = text.substr(selbeg, selend - selbeg + 1);
        clip::set_text(base::to_utf8(selected));
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

    case EntryCmd::DeleteBackwardWord:
      m_select = m_caret;
      backwardWord();
      if (m_caret < m_select)
        text.erase(m_caret, m_select-m_caret);
      m_select = -1;
      break;

    case EntryCmd::DeleteForwardToEndOfLine:
      text.erase(m_caret);
      break;

    case EntryCmd::SelectAll:
      selectAllText();
      break;
  }

  std::string newText = base::to_utf8(text);
  if (newText != this->text()) {
    setText(newText.c_str());
    onChange();
  }

  setCaretPos(m_caret);
  invalidate();
}

#define IS_WORD_CHAR(ch)                                \
  (!((!ch) || (std::isspace(ch)) ||                     \
    ((ch) == '/') || ((ch) == '\\')))

void Entry::forwardWord()
{
  base::utf8_const_iterator utf8_begin = base::utf8_const_iterator(text().begin());
  int textlen = base::utf8_length(text());
  int ch;

  for (; m_caret < textlen; m_caret++) {
    ch = *(utf8_begin + m_caret);
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; m_caret < textlen; m_caret++) {
    ch = *(utf8_begin + m_caret);
    if (!IS_WORD_CHAR(ch)) {
      ++m_caret;
      break;
    }
  }
}

void Entry::backwardWord()
{
  base::utf8_const_iterator utf8_begin = base::utf8_const_iterator(text().begin());
  int ch;

  for (--m_caret; m_caret >= 0; --m_caret) {
    ch = *(utf8_begin + m_caret);
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; m_caret >= 0; --m_caret) {
    ch = *(utf8_begin + m_caret);
    if (!IS_WORD_CHAR(ch)) {
      ++m_caret;
      break;
    }
  }

  if (m_caret < 0)
    m_caret = 0;
}

int Entry::getAvailableTextLength()
{
  return clientChildrenBounds().w / font()->charWidth('w');
}

bool Entry::isPosInSelection(int pos)
{
  return (pos >= MIN(m_caret, m_select) && pos <= MAX(m_caret, m_select));
}

void Entry::showEditPopupMenu(const gfx::Point& pt)
{
  Menu menu;
  MenuItem cut("Cut");
  MenuItem copy("Copy");
  MenuItem paste("Paste");
  menu.addChild(&cut);
  menu.addChild(&copy);
  menu.addChild(&paste);
  cut.Click.connect(base::Bind(&Entry::executeCmd, this, EntryCmd::Cut, 0, false));
  copy.Click.connect(base::Bind(&Entry::executeCmd, this, EntryCmd::Copy, 0, false));
  paste.Click.connect(base::Bind(&Entry::executeCmd, this, EntryCmd::Paste, 0, false));

  if (isReadOnly()) {
    cut.setEnabled(false);
    paste.setEnabled(false);
  }

  menu.showPopup(pt);
}

} // namespace ui
