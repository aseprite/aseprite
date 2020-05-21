// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/entry.h"

#include "base/bind.h"
#include "base/clamp.h"
#include "base/string.h"
#include "os/draw_text.h"
#include "os/font.h"
#include "os/system.h"
#include "ui/manager.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>

namespace ui {

Entry::Entry(const int maxsize, const char* format, ...)
  : Widget(kEntryWidget)
  , m_timer(500, this)
  , m_maxsize(maxsize)
  , m_caret(0)
  , m_scroll(0)
  , m_select(0)
  , m_hidden(false)
  , m_state(false)
  , m_readonly(false)
  , m_recent_focused(false)
  , m_lock_selection(false)
  , m_translate_dead_keys(true)
{
  enableFlags(CTRL_RIGHT_CLICK);

  // formatted string
  char buf[4096];               // TODO buffer overflow
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

void Entry::setMaxTextLength(const int maxsize)
{
  m_maxsize = maxsize;
}

bool Entry::isReadOnly() const
{
  return m_readonly;
}

void Entry::setReadOnly(bool state)
{
  m_readonly = state;
}

void Entry::showCaret()
{
  m_hidden = false;
  if (shouldStartTimer(hasFocus()))
    m_timer.start();
  invalidate();
}

void Entry::hideCaret()
{
  m_hidden = true;
  m_timer.stop();
  invalidate();
}

int Entry::lastCaretPos() const
{
  return int(m_boxes.size()-1);
}

void Entry::setCaretPos(int pos)
{
  gfx::Size caretSize = theme()->getEntryCaretSize(this);
  int textlen = lastCaretPos();
  m_caret = base::clamp(pos, 0, textlen);
  m_scroll = base::clamp(m_scroll, 0, textlen);

  // Backward scroll
  if (m_caret < m_scroll)
    m_scroll = m_caret;
  // Forward scroll
  else if (m_caret > m_scroll) {
    int xLimit = bounds().x2() - border().right();
    while (m_caret > m_scroll) {
      int segmentWidth = 0;
      for (int j=m_scroll; j<m_caret; ++j)
        segmentWidth += m_boxes[j].width;

      int x = bounds().x + border().left() + segmentWidth + caretSize.w;
      if (x < xLimit)
        break;
      else
        ++m_scroll;
    }
  }

  if (shouldStartTimer(hasFocus()))
    m_timer.start();
  m_state = true;

  invalidate();
}

void Entry::setCaretToEnd()
{
  int end = lastCaretPos();
  selectText(end, end);
}

void Entry::selectText(int from, int to)
{
  int end = lastCaretPos();

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

std::string Entry::selectedText() const
{
  Range range = selectedRange();
  if (!range.isEmpty())
    return text().substr(m_boxes[range.from].from,
                         m_boxes[range.to-1].to - m_boxes[range.from].from);
  else
    return std::string();
}

Entry::Range Entry::selectedRange() const
{
  Range range;
  if ((m_select >= 0) &&
      (m_caret != m_select)) {
    range.from = std::min(m_caret, m_select);
    range.to   = std::max(m_caret, m_select);

    ASSERT(range.from >= 0 && range.from < int(m_boxes.size()));
    ASSERT(range.to   >= 0 && range.to   <= int(m_boxes.size()));
  }
  return range;
}

void Entry::setSuffix(const std::string& suffix)
{
  if (m_suffix != suffix) {
    m_suffix = suffix;
    invalidate();
  }
}

void Entry::setTranslateDeadKeys(bool state)
{
  m_translate_dead_keys = state;
}

void Entry::getEntryThemeInfo(int* scroll, int* caret, int* state, Range* range) const
{
  if (scroll) *scroll = m_scroll;
  if (caret) *caret = m_caret;
  if (state) *state = !m_hidden && m_state;
  if (range) *range = selectedRange();
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
      if (shouldStartTimer(true))
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

      // Start processing dead keys
      if (m_translate_dead_keys)
        os::instance()->setTranslateDeadKeys(true);
      break;

    case kFocusLeaveMessage:
      invalidate();

      m_timer.stop();

      if (!m_lock_selection)
        deselectText();

      m_recent_focused = false;

      // Stop processing dead keys
      if (m_translate_dead_keys)
        os::instance()->setTranslateDeadKeys(false);
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
            if (msg->ctrlPressed() || msg->altPressed())
              cmd = EntryCmd::DeleteBackwardWord;
            else
              cmd = EntryCmd::DeleteBackward;
            break;

          default:
            // Map common macOS/Windows shortcuts for Cut/Copy/Paste/Select all
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
            break;
        }

        if (cmd == EntryCmd::NoOp) {
          if (keymsg->unicodeChar() >= 32) {
            executeCmd(EntryCmd::InsertChar, keymsg->unicodeChar(),
                       (msg->shiftPressed()) ? true: false);

            // Select dead-key
            if (keymsg->isDeadKey()) {
              if (lastCaretPos() < m_maxsize)
                selectText(m_caret-1, m_caret);
            }
            return true;
          }
          // Consume all key down of modifiers only, e.g. so the user
          // can press first "Ctrl" key, and then "Ctrl+C"
          // combination.
          else if (keymsg->scancode() >= kKeyFirstModifierScancode) {
            return true;
          }
          else {
            break;              // Propagate to manager
          }
        }

        executeCmd(cmd, keymsg->unicodeChar(),
                   (msg->shiftPressed()) ? true: false);
        return true;
      }
      break;

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        bool is_dirty = false;
        int c = getCaretFromMouse(static_cast<MouseMessage*>(msg));

        if (static_cast<MouseMessage*>(msg)->left() || !isPosInSelection(c)) {
          // Move caret
          if (m_caret != c) {
            setCaretPos(c);
            is_dirty = true;
            invalidate();
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
          if (shouldStartTimer(true))
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

// static
gfx::Size Entry::sizeHintWithText(Entry* entry,
                                  const std::string& text)
{
  int w =
    entry->font()->textLength(text) +
    + 2*entry->theme()->getEntryCaretSize(entry).w
    + entry->border().width();

  w = std::min(w, ui::display_w()/2);

  int h =
    + entry->font()->height()
    + entry->border().height();

  return gfx::Size(w, h);
}

void Entry::onSizeHint(SizeHintEvent& ev)
{
  int trailing = font()->textLength(getSuffix());
  trailing = std::max(trailing, 2*theme()->getEntryCaretSize(this).w);

  int w =
    font()->textLength("w") * std::min(m_maxsize, 6) +
    + trailing
    + border().width();

  w = std::min(w, ui::display_w()/2);

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
  recalcCharBoxes(text());

  int textlen = lastCaretPos();
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
  int mouseX = mousemsg->position().x;
  if (mouseX < bounds().x+border().left()) {
    // Scroll to the left
    return std::max(0, m_scroll-1);
  }

  int lastPos = lastCaretPos();
  int i = std::min(m_scroll, lastPos);
  for (; i<lastPos; ++i) {
    int segmentWidth = 0;
    int indexBox = 0;
    for (int j=m_scroll; j<i; ++j) {
      segmentWidth += m_boxes[j].width;
      indexBox = j+1;
    }

    int x = bounds().x + border().left() + segmentWidth + m_boxes[indexBox].width / 2;

    if (mouseX > bounds().x2() - border().right()) {
      if (x >= bounds().x2() - border().right()) {
        // Scroll to the right
        break;
      }
    }
    else if (x > mouseX)
      break;
  }

  return base::clamp(i, 0, lastPos);
}

void Entry::executeCmd(EntryCmd cmd, int unicodeChar, bool shift_pressed)
{
  std::string text = this->text();
  const Range range = selectedRange();

  switch (cmd) {

    case EntryCmd::NoOp:
      break;

    case EntryCmd::InsertChar:
      // delete the entire selection
      if (!range.isEmpty()) {
        deleteRange(range, text);

        // We set the caret to the beginning of the erased selection,
        // needed to show the first inserted character in case
        // m_scroll > m_caret. E.g. we select all text and insert a
        // new character to replace the whole text, the new inserted
        // character makes m_caret=1, so m_scroll will be 1 too, but
        // we need to make m_scroll=0 to show the new inserted char.)
        // In this way, we first ensure a m_scroll value enough to
        // show the new inserted character.
        recalcCharBoxes(text);
        setCaretPos(m_caret);
      }

      // Convert the unicode character -> wstring -> utf-8 string -> insert the utf-8 string
      if (lastCaretPos() < m_maxsize) {
        ASSERT(m_caret <= lastCaretPos());

        std::wstring unicodeStr;
        unicodeStr.push_back(unicodeChar);

        text.insert(m_boxes[m_caret].from,
                    base::to_utf8(unicodeStr));
        recalcCharBoxes(text);
        ++m_caret;
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

      m_caret = lastCaretPos();
      break;

    case EntryCmd::DeleteForward:
    case EntryCmd::Cut:
      // delete the entire selection
      if (!range.isEmpty()) {
        // *cut* text!
        if (cmd == EntryCmd::Cut)
          set_clipboard_text(selectedText());

        // remove text
        deleteRange(range, text);
      }
      // delete the next character
      else {
        if (m_caret < (int)text.size())
          text.erase(m_boxes[m_caret].from,
                     m_boxes[m_caret].to - m_boxes[m_caret].from);
      }

      m_select = -1;
      break;

    case EntryCmd::Paste: {
      std::string clipboard;
      if (get_clipboard_text(clipboard)) {
        // delete the entire selection
        if (!range.isEmpty()) {
          deleteRange(range, text);
          m_select = -1;
        }

        // Paste text
        recalcCharBoxes(text);
        int oldBoxes = m_boxes.size();

        text.insert(m_boxes[m_caret].from, clipboard);

        // Remove extra chars that do not fit in m_maxsize
        recalcCharBoxes(text);
        if (lastCaretPos() > m_maxsize) {
          text.erase(m_boxes[m_maxsize].from,
                     text.size() - m_boxes[m_maxsize].from);
          recalcCharBoxes(text);
        }

        int newBoxes = m_boxes.size();
        setCaretPos(m_caret+(newBoxes - oldBoxes));
      }
      break;
    }

    case EntryCmd::Copy:
      if (!range.isEmpty())
        set_clipboard_text(selectedText());
      break;

    case EntryCmd::DeleteBackward:
      // delete the entire selection
      if (!range.isEmpty()) {
        deleteRange(range, text);
      }
      // delete the previous character
      else {
        if (m_caret > 0) {
          --m_caret;
          text.erase(m_boxes[m_caret].from,
                     m_boxes[m_caret].to - m_boxes[m_caret].from);
        }
      }

      m_select = -1;
      break;

    case EntryCmd::DeleteBackwardWord:
      m_select = m_caret;
      backwardWord();
      if (m_caret < m_select) {
        text.erase(m_boxes[m_caret].from,
                   m_boxes[m_select-1].to - m_boxes[m_caret].from);
      }
      m_select = -1;
      break;

    case EntryCmd::DeleteForwardToEndOfLine:
      text.erase(m_boxes[m_caret].from);
      break;

    case EntryCmd::SelectAll:
      selectAllText();
      break;
  }

  if (text != this->text()) {
    setText(text);
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
  int textlen = lastCaretPos();
  int ch;

  for (; m_caret < textlen; ++m_caret) {
    ch = m_boxes[m_caret].codepoint;
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; m_caret < textlen; ++m_caret) {
    ch = m_boxes[m_caret].codepoint;
    if (!IS_WORD_CHAR(ch)) {
      ++m_caret;
      break;
    }
  }
}

void Entry::backwardWord()
{
  int ch;

  for (--m_caret; m_caret >= 0; --m_caret) {
    ch = m_boxes[m_caret].codepoint;
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; m_caret >= 0; --m_caret) {
    ch = m_boxes[m_caret].codepoint;
    if (!IS_WORD_CHAR(ch)) {
      ++m_caret;
      break;
    }
  }

  if (m_caret < 0)
    m_caret = 0;
}

bool Entry::isPosInSelection(int pos)
{
  return (pos >= std::min(m_caret, m_select) &&
          pos <= std::max(m_caret, m_select));
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

class Entry::CalcBoxesTextDelegate : public os::DrawTextDelegate {
public:
  CalcBoxesTextDelegate(const int end) : m_end(end) {
  }

  const Entry::CharBoxes& boxes() const { return m_boxes; }

  void preProcessChar(const int index,
                      const int codepoint,
                      gfx::Color& fg,
                      gfx::Color& bg,
                      const gfx::Rect& charBounds) override {
    if (!m_boxes.empty())
      m_boxes.back().to = index;

    m_box = CharBox();
    m_box.codepoint = codepoint;
    m_box.from = index;
    m_box.to = m_end;
  }

  bool preDrawChar(const gfx::Rect& charBounds) override {
    m_box.width = charBounds.w;
    return true;
  }

  void postDrawChar(const gfx::Rect& charBounds) override {
    m_boxes.push_back(m_box);
  }

private:
  Entry::CharBox m_box;
  Entry::CharBoxes m_boxes;
  int m_end;
};

void Entry::recalcCharBoxes(const std::string& text)
{
  int lastTextIndex = int(text.size());
  CalcBoxesTextDelegate delegate(lastTextIndex);
  os::draw_text(nullptr, font(),
                 base::utf8_const_iterator(text.begin()),
                 base::utf8_const_iterator(text.end()),
                 gfx::ColorNone, gfx::ColorNone, 0, 0, &delegate);
  m_boxes = delegate.boxes();

  if (!m_boxes.empty()) {
    m_boxes.back().to = lastTextIndex;
  }

  // A last box for the last position
  CharBox box;
  box.codepoint = 0;
  box.from = box.to = lastTextIndex;
  m_boxes.push_back(box);
}

bool Entry::shouldStartTimer(bool hasFocus)
{
  return (!m_hidden && hasFocus && isEnabled());
}

void Entry::deleteRange(const Range& range, std::string& text)
{
  text.erase(m_boxes[range.from].from,
             m_boxes[range.to-1].to - m_boxes[range.from].from);
  m_caret = range.from;
}

} // namespace ui
