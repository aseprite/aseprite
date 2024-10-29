// Aseprite UI Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/entry.h"

#include "base/string.h"
#include "base/split_string.h"
#include "base/replace_string.h"
#include "base/utf8_decode.h"
#include "os/system.h"
#include "text/draw_text.h"
#include "text/font.h"
#include "ui/display.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <memory>

namespace ui {

// Shared timer between all entries.
static std::unique_ptr<Timer> s_timer;

static inline bool is_word_char(int ch) {
  return (ch && !std::isspace(ch) && !std::ispunct(ch) && ch != '\n');
}

Entry::Entry(const int maxsize, const char* format, ...)
  : Widget(kEntryWidget)
  , m_maxsize(maxsize)
  , m_caret(0)
  , m_select(0)
  , m_hidden(false)
  , m_state(false)
  , m_readonly(false)
  , m_recentlyFocused(false)
  , m_lockSelection(false)
  , m_persistSelection(false)
  , m_translateDeadKeys(true)
  , m_scale(1.0f, 1.0f)
  , m_lines(0)
  , m_multiline(false)
  , m_horizontalScroll(0)
  , m_verticalLineScroll(0)
{
  enableFlags(CTRL_RIGHT_CLICK);

  // formatted string
  char buf[4096];               // TODO buffer overflow
  if (format) {
    va_list ap;
    va_start(ap, format);
    std::vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
  }
  // empty string
  else {
    buf[0] = 0;
  }

  // TODO: support for text alignment
  // widget->align = LEFT | MIDDLE;
  setText(buf);

  setFocusStop(true);
  initTheme();
}

Entry::~Entry()
{
  stopTimer();
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

bool Entry::isMultiline() const
{
  return m_multiline;
}

void Entry::setMultiline(bool state)
{
  m_multiline = state;
  ASSERT(m_multiline && !m_suffix);
}

void Entry::showCaret()
{
  m_hidden = false;
  if (shouldStartTimer(hasFocus()))
    startTimer();
  invalidate();
}

void Entry::hideCaret()
{
  m_hidden = true;
  stopTimer();
  invalidate();
}

int Entry::lastCaretPos() const
{
  return m_boxes.empty() ? 0 : int(m_boxes.size() - 1);
}

int Entry::caretToLine(const int caret_position)
{
  if (!m_multiline)
    return 0;

  int i = 0;
  for (const auto& pos : m_lines) {
    if (caret_position >= pos.start && caret_position < pos.end)
      return i;

    i++;
  }

  return i;
}

void Entry::setCaretPos(const int pos)
{
  gfx::Size caretSize = theme()->getEntryCaretSize(this);
  int textlen = lastCaretPos();
  m_caret = std::clamp(pos, 0, textlen);
  
  int currentLine = caretToLine(m_caret);
  const float visibleTextHeight = bounds().h - border().height();

  if ((currentLine * textHeight() - m_verticalLineScroll * textHeight()) >
      visibleTextHeight) {
    // Need vertical scroll
    m_verticalLineScroll += 1;
  }
  else if (currentLine < m_verticalLineScroll) {
      m_verticalLineScroll = currentLine;
  }
    
  if (!m_boxes.empty() && (m_boxes[m_caret].x + m_boxes[m_caret].width) >
      bounds().w - border().width()) {
    m_horizontalScroll = std::clamp(m_horizontalScroll, 0, textlen);

    // Backward scroll
    if (m_caret < m_horizontalScroll) {
      m_horizontalScroll = m_caret;
    }
    // Forward scroll
    else if (m_caret > m_horizontalScroll) {
      const int xLimit = bounds().x2() - border().right();

      while (m_caret > m_horizontalScroll) {
        const float visibleTextWidth =
          m_boxes[m_caret].x - m_boxes[m_horizontalScroll].x;

        const int x = bounds().x + border().left() +
                      visibleTextWidth * m_scale.x + caretSize.w;

        if (x < xLimit)
          break;
        else
          ++m_horizontalScroll;
      }
    }
  }
  else
    m_horizontalScroll = 0;

  if (shouldStartTimer(hasFocus()))
    startTimer();

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
  if (!range.isEmpty()) {
    auto selected = text().substr(m_boxes[range.from].from, m_boxes[range.to - 1].to - m_boxes[range.from].from);
    
    #if LAF_WINDOWS
    base::replace_string(selected, "\n", "\r\n");
    #endif

    return selected;
  }

  return std::string();
}

Entry::Range Entry::selectedRange() const
{
  Range range;
  if ((m_select >= 0) &&
      (m_caret != m_select)) {
    range.from = std::min(m_caret, m_select);
    range.to   = std::max(m_caret, m_select);

    range.from = std::clamp(range.from, 0, std::max(0, int(m_boxes.size())-1));
    range.to = std::clamp(range.to, 0, int(m_boxes.size()));
  }
  return range;
}

void Entry::setSuffix(const std::string& suffix)
{
  // No-op cases
  if ((!m_suffix && suffix.empty()) ||
      (m_suffix && *m_suffix == suffix) ||
      m_multiline)
    return;

  m_suffix = std::make_unique<std::string>(suffix);
  invalidate();
}

std::string Entry::getSuffix()
{
  return (m_suffix && !m_multiline ? *m_suffix: std::string());
}

void Entry::setTranslateDeadKeys(bool state)
{
  m_translateDeadKeys = state;
}

void Entry::getEntryThemeInfo(int* scroll, int* line_scroll, int* caret, int* state, Range* range) const
{
  if (scroll != nullptr)
    *scroll = m_horizontalScroll;
  if (line_scroll != nullptr)
    *line_scroll = m_verticalLineScroll;
  if (caret != nullptr)
    *caret = m_caret;
  if (state != nullptr)
    *state = !m_hidden && m_state;
  if (range != nullptr)
    *range = selectedRange();
}

gfx::Rect Entry::getEntryTextBounds() const
{
  return onGetEntryTextBounds();
}

gfx::Rect Entry::getCharBoxBounds(const int i)
{
  ASSERT(i >= 0 && i < int(m_boxes.size()));
  if (i >= 0 && i < int(m_boxes.size()))
    return gfx::Rect(m_boxes[i].x, m_boxes[i].y, m_boxes[i].width, textHeight());
  else
    return gfx::Rect();
}

bool Entry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kTimerMessage:
      if (hasFocus() &&
          static_cast<TimerMessage*>(msg)->timer() == s_timer.get()) {
        // Blinking caret
        m_state = (m_state ? false : true);
        invalidate();
      }
      break;

    case kFocusEnterMessage:
      if (shouldStartTimer(true))
        startTimer();

      m_state = true;
      invalidate();

      if (m_lockSelection) {
        m_lockSelection = false;
      }
      else {
        if (!m_persistSelection && !m_multiline) // TODO: ?
          selectAllText();
        m_recentlyFocused = true;
      }

      // Start processing dead keys
      if (m_translateDeadKeys)
        os::System::instance()->setTranslateDeadKeys(true);
      break;

    case kFocusLeaveMessage:
      invalidate();

      stopTimer();

      if (!m_lockSelection && !m_persistSelection)
        deselectText();

      m_recentlyFocused = false;

      // Stop processing dead keys
      if (m_translateDeadKeys)
        os::System::instance()->setTranslateDeadKeys(false);
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

          case kKeyEnter:
            if (m_multiline)
              cmd = EntryCmd::InsertNewLine;
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

          case kKeyUp:
            if (m_multiline)
              cmd = EntryCmd::LineUp;
            break;

          case kKeyDown:
            if (m_multiline)
              cmd = EntryCmd::LineDown;
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
                case kKeyX:
                  cmd = EntryCmd::Cut;
                  break;
                case kKeyC:
                  cmd = EntryCmd::Copy;
                  break;
                case kKeyV:
                  cmd = EntryCmd::Paste;
                  break;
                case kKeyA:
                  cmd = EntryCmd::SelectAll;
                  break;
              }
            }
            break;
        }

        if (cmd == EntryCmd::NoOp) {
          if (keymsg->unicodeChar() >= 32) {
            executeCmd(EntryCmd::InsertChar,
                       keymsg->unicodeChar(),
                       msg->shiftPressed());

            // Select dead-key
            if (keymsg->isDeadKey()) {
              if (lastCaretPos() < m_maxsize)
                selectText(m_caret - 1, m_caret);
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
            break;  // Propagate to manager
          }
        }

        executeCmd(cmd, keymsg->unicodeChar(), msg->shiftPressed());
        return true;
      }
      break;

    case kMouseDownMessage:
      captureMouse();

      // Disable selecting words if we click again (only
      // double-clicking enables selecting words again).
      if (!m_selectingWords.isEmpty())
        m_selectingWords.reset();

      [[fallthrough]];

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
          if (m_recentlyFocused) {
            m_recentlyFocused = false;
            m_select = m_caret;
          }
          // Deselect
          else if (msg->type() == kMouseDownMessage) {
            m_select = m_caret;
          }
          // Continue selecting words
          else if (!m_selectingWords.isEmpty()) {
            Range toWord = wordRange(m_caret);
            if (toWord.from < m_selectingWords.from) {
              m_select = std::max(m_selectingWords.to, toWord.to);
              setCaretPos(std::min(m_selectingWords.from, toWord.from));
            }
            else {
              m_select = std::min(m_selectingWords.from, toWord.from);
              setCaretPos(std::max(m_selectingWords.to, toWord.to));
            }
          }
        }

        // Show the caret
        if (is_dirty) {
          if (shouldStartTimer(true))
            startTimer();
          m_state = true;
        }

        return true;
      }
      break;

    case kMouseWheelMessage: {
      if (!m_multiline)
        break;

      auto mouseMsg = static_cast<MouseMessage*>(msg);
      const float visibleTextHeight = bounds().h - border().height();
      const float visibleTextWidth = bounds().w - border().width();
      const int lineHeight = textHeight() * m_scale.y;

      if (m_lines.size() * lineHeight > visibleTextHeight) {
        if (mouseMsg->wheelDelta().y < 0 && m_verticalLineScroll > 0) {
          m_verticalLineScroll -= 1;
        }
        else if (mouseMsg->wheelDelta().y > 0 && m_verticalLineScroll < m_lines.size() - 1) {
          m_verticalLineScroll += 1;
        }

        invalidate();
      }

      // TODO: Support horizontal scroll wheels
    }
    break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        if (!m_selectingWords.isEmpty())
          m_selectingWords.reset();

        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        if (mouseMsg->right()) {
          // This flag is disabled in kFocusEnterMessage message handler.
          m_lockSelection = true;

          showEditPopupMenu(mouseMsg->position());
          requestFocus();
        }
      }
      return true;

    case kDoubleClickMessage:
      if (!hasFocus())
        requestFocus();

      m_selectingWords = wordRange(m_caret);
      selectText(m_selectingWords.from, m_selectingWords.to);

      // Capture mouse to continue selecting words on kMouseMoveMessage
      captureMouse();
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

  w = std::min(w, entry->display()->workareaSizeUIScale().w/2);

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

  w = std::min(w, display()->workareaSizeUIScale().w/2);

  int h =
    + font()->height()
    + border().height();

  ev.setSizeHint(w, h);
}

void Entry::onPaint(PaintEvent& ev)
{
  theme()->paintEntry(ev);
}

void Entry::onSetFont()
{
  Widget::onSetFont();
  recalcCharBoxes(text());
}

void Entry::onSetText()
{
  Widget::onSetText();
  recalcCharBoxes(text());

  if (m_multiline) {
    m_lines.clear();
    std::vector<std::string> lines;
    base::split_string(text(), lines, "\n");

    int pos = 0;
    int i = 0;
    for (const auto& line : lines) {
      int length = line.size();
      if (int(lines.size()) > i + 1)
        length += 1; // Trailing newline.

      EntryLine lineInfo{ pos, pos + length };
      pos = lineInfo.end;
      m_lines.push_back(lineInfo);
    }
  }

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
  bounds.y += border().top();
  bounds.w -= border().right() + (border().width() * 2);
  bounds.h = (textHeight() * int(m_lines.size())) - (border().height() * 2);
  return bounds;
}

int Entry::getCaretFromMouse(MouseMessage* mouseMsg)
{
  gfx::Point position = mouseMsg->position();

  int line = caretToLine(m_caret);
  int lineHeight = textHeight() * m_scale.y;
  int currentLineStartY = bounds().y + line * lineHeight;
  int currentLineEndY = currentLineStartY + lineHeight;

  if (position.y > currentLineStartY && position.y < currentLineEndY) {
    // Mouse position is within a single line:
    if (position.x < bounds().x + border().left())
      return (m_horizontalScroll == 0) ? m_caret : std::max(0, m_caret - 1);

    if (position.x > bounds().x2() && m_lines[line].end > m_caret)
      return m_caret + 1;
  }

  if (bounds().contains(position)) {
    gfx::Point offsetPosition(position.x - (bounds().x + border().left()),
                              position.y + (m_verticalLineScroll * lineHeight) - (bounds().y + border().top()));

    
    if (m_horizontalScroll > 0) {
      int length = font()->textLength(text().substr(m_multiline ? m_lines[line].start : 0, m_horizontalScroll));
      ASSERT(length > 0);

      offsetPosition.x += length;
    }

    for (const auto& box : m_boxes) {
      const gfx::Rect boxRect(box.x,
                              box.y,
                              box.width,
                              lineHeight);

      if (boxRect.contains(offsetPosition)) {
        return box.from;
      }
    }

    // If we can't find an exact match, attempt to get the line, if any.
    if (m_multiline && m_lines.size() > 1) {
      int lineY = currentLineStartY;

      for (auto it = m_lines.begin() + m_verticalLineScroll;
           it != m_lines.end();
           ++it) {
        if (position.y >= lineY && position.y <= lineY + lineHeight) {
          return (*it).end - 1;
        }

        lineY += lineHeight;
      }
    }
  }

  if (!m_multiline) // Blank clicking on non-multiline
    return lastCaretPos();

  return m_caret;
}

void Entry::executeCmd(const EntryCmd cmd,
                       const base::codepoint_t unicodeChar,
                       const bool shift_pressed)
{
  std::string text = this->text();
  const Range range = selectedRange();

  switch (cmd) {
    case EntryCmd::NoOp:
      break;

    case EntryCmd::InsertChar:
    case EntryCmd::InsertNewLine:
      // delete the entire selection
      if (!range.isEmpty()) {
        deleteRange(range, text);

        // TODO: Rewrite/reanalyize this.
        //
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
        std::string insert = (cmd == EntryCmd::InsertChar) ?
                               base::codepoint_to_utf8(unicodeChar) :
                               "\n";

        if (m_caret == lastCaretPos()) {
          text.append(insert);
        }
        else {
          text.insert(m_boxes[m_caret].from, insert);
        }

        ++m_caret;
        recalcCharBoxes(text);
        setCaretPos(m_caret);
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
        
        deleteRange(range, text);
        recalcCharBoxes(text);
      }
      // delete the next character
      else {
        if (m_caret < (int)text.size())
          deleteRange(Range{ m_caret, m_caret + 1 }, text);
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

        #if LAF_WINDOWS
        base::replace_string(clipboard, "\r\n", "\n");
        #endif

        // Paste text
        recalcCharBoxes(text);
        int oldBoxes = m_boxes.size();

        text.insert(m_boxes[m_caret].from, clipboard);

        // Remove extra chars that do not fit in m_maxsize
        recalcCharBoxes(text);
        if (lastCaretPos() > m_maxsize) {
          text.erase(text.begin() + m_boxes[m_maxsize].from, text.end());
          recalcCharBoxes(text);
        }

        int newBoxes = m_boxes.size();
        setCaretPos(m_caret + (newBoxes - oldBoxes));
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
          deleteRange(Range{ m_caret - 1, m_caret }, text);
        }
      }

      m_select = -1;
      break;

    case EntryCmd::DeleteBackwardWord:
      m_select = m_caret;
      backwardWord();
      if (m_caret < m_select) {
        deleteRange(Range{ m_caret, m_select }, text); // TODO: Test
      }
      m_select = -1;
      break;

    case EntryCmd::DeleteForwardToEndOfLine: {
      int line = caretToLine(m_caret);
      deleteRange(Range{ m_caret, m_lines[line].end - 1}, text);
    } break;

    case EntryCmd::SelectAll:
      selectAllText();
      break;

    case EntryCmd::LineUp:
    case EntryCmd::LineDown: {
      if (m_lines.size() == 1)
        break;

      if (shift_pressed) {
        if (m_select < 0)
          m_select = m_caret;
      }
      else
        m_select = -1;

      int line = caretToLine(m_caret);
      EntryLine lineInfo;

      if (cmd == EntryCmd::LineUp) {
        if (line <= 0)
          break;

        lineInfo = m_lines[line - 1];
      }
      else {
        if (line >= m_lines.size() - 1)
          break;

        lineInfo = m_lines[line + 1];
      }

      int offset = m_caret - m_lines[line].start;

      m_caret = lineInfo.start;
      
      if (m_caret + offset < lineInfo.end) {
        m_caret += offset;
      }
      else {
        // If we have a 1-character line, use the start, otherwise the end.
        m_caret = (lineInfo.start + 1 == lineInfo.end) ? lineInfo.start : lineInfo.end;
      }
    }
    break;
  }

  if (text != this->text()) {
    setText(text);
    onChange();
  }

  setCaretPos(m_caret);
  invalidate();
}

void Entry::forwardWord()
{
  int textlen = lastCaretPos();

  for (; m_caret < textlen; ++m_caret) {
    if (is_word_char(m_boxes[m_caret].codepoint))
      break;
  }

  for (; m_caret < textlen; ++m_caret) {
    if (!is_word_char(m_boxes[m_caret].codepoint))
      break;
  }
}

void Entry::backwardWord()
{
  for (--m_caret; m_caret >= 0; --m_caret) {
    if (is_word_char(m_boxes[m_caret].codepoint))
      break;
  }

  for (; m_caret >= 0; --m_caret) {
    if (!is_word_char(m_boxes[m_caret].codepoint)) {
      ++m_caret;
      break;
    }
  }

  if (m_caret < 0)
    m_caret = 0;
}

Entry::Range Entry::wordRange(int pos)
{
  const int last = lastCaretPos();
  pos = std::clamp(pos, 0, last);

  int i, j;
  i = j = pos;

  // Select word space
  if (is_word_char(m_boxes[pos].codepoint)) {
    for (; i>=0; --i) {
      if (!is_word_char(m_boxes[i].codepoint))
        break;
    }
    ++i;
    for (; j<=last; ++j) {
      if (!is_word_char(m_boxes[j].codepoint))
        break;
    }
  }
  // Select punctuation space
  else {
    for (; i>=0; --i) {
      if (is_word_char(m_boxes[i].codepoint))
        break;
    }
    ++i;
    for (; j<=last; ++j) {
      if (is_word_char(m_boxes[j].codepoint))
        break;
    }
  }
  return Range(i, j);
}

bool Entry::isPosInSelection(int pos) const
{
  return (pos >= std::min(m_caret, m_select) &&
          pos <= std::max(m_caret, m_select));
}

void Entry::showEditPopupMenu(const gfx ::Point& pt)
{
  Menu menu;
  MenuItem cut("Cut"); // TODO: localization? Strings::commands_Cut()?
  MenuItem copy("Copy");
  MenuItem paste("Paste");
  menu.addChild(&cut);
  menu.addChild(&copy);
  menu.addChild(&paste);
  cut.Click.connect([this]{ executeCmd(EntryCmd::Cut, 0, false); });
  copy.Click.connect([this]{ executeCmd(EntryCmd::Copy, 0, false); });
  paste.Click.connect([this]{ executeCmd(EntryCmd::Paste, 0, false); });

  if (isReadOnly()) {
    cut.setEnabled(false);
    paste.setEnabled(false);
  }

  menu.showPopup(pt, display());
}

class Entry::CalcBoxesTextDelegate : public text::DrawTextDelegate {
public:
  CalcBoxesTextDelegate(const int end)
    : m_end(end)
    , m_lineLength(0)
    , m_lineY(0)
  {
  }

  const Entry::CharBoxes& boxes() const {
    return m_boxes;
  }

  void preProcessChar(const int index,
                      const base::codepoint_t codepoint,
                      gfx::Color& fg,
                      gfx::Color& bg,
                      const gfx::Rect& charBounds) override {
    if (!m_boxes.empty())
      m_boxes.back().to = m_lineLength + index;

    m_box = CharBox();
    m_box.codepoint = codepoint;
    m_box.from = m_lineLength + index;
    m_box.to = m_end;

    if (m_box.from > m_box.to)
      m_box.from = m_box.to;

    m_box.x = 0;
    m_box.y = m_lineY;
  }

  bool preDrawChar(const gfx::Rect& charBounds) override {
    m_box.x += charBounds.x;
    m_box.y += charBounds.y;
    m_box.width = charBounds.w;
    return true;
  }

  void postDrawChar(const gfx::Rect& charBounds) override {
    m_boxes.push_back(m_box);
  }

  void nextLine(std::string prevLine, int caretWidth, int height) {
    m_lineLength += prevLine.size() + 1;
    m_lineY += height;

    CharBox box;
    box.codepoint = 20; // Space codepoint (TODO?)
    box.width = caretWidth;

    if (m_boxes.empty()) {
      box.from = 0;
      box.to = 0;
      box.x = 0;
      box.y = m_lineY;
    }
    else {
      const auto& lastBox = m_boxes.back();
      box.from = lastBox.from + 1;
      box.to = box.from + 1;
      box.x = lastBox.x + lastBox.width;
      box.y = lastBox.y;
    }

    m_boxes.push_back(box);
  }

private:
  Entry::CharBox m_box;
  Entry::CharBoxes m_boxes;
  int m_end;
  int m_lineLength;
  int m_lineY;
};

void Entry::recalcCharBoxes(const std::string& text)
{
  if (text.empty()) {
    m_boxes.clear(); 
    return;
  }

  std::vector<std::string> lines;
  if (m_multiline)
    base::split_string(text, lines, "\n");
  else
    lines.push_back(text);

  gfx::Rect bounds = getEntryTextBounds();

  int caretWidth = theme()->getEntryCaretSize(this).w;
  int lastTextIndex = int(text.size());
  CalcBoxesTextDelegate delegate(lastTextIndex);

  for (const auto& textString : lines) {
    gfx::Rect rect;

    if (!textString.empty())
      rect = text::draw_text(nullptr,
        theme()->fontMgr(),
        base::AddRef(font()),
        textString,
        gfx::ColorNone,
        gfx::ColorNone,
        0,
        0,
        &delegate,
        onGetTextShaperFeatures());

    delegate.nextLine(
      textString,
      caretWidth,  
      textHeight() * m_scale.y
    );
  }

  m_boxes = delegate.boxes();

  if (!m_boxes.empty()) {
    m_boxes.back().to = lastTextIndex;
  }
}

bool Entry::shouldStartTimer(bool hasFocus)
{
  return (!m_hidden && hasFocus && isEnabled());
}

void Entry::deleteRange(const Range& range, std::string& text)
{
  if (range.size() <= 0)
    return;

  text.erase(text.begin() + m_boxes[range.from].from, // TODO: Unicode/accented characters fucks this up?
             text.begin() + m_boxes[range.to].from);
  m_caret = range.from;
}

void Entry::startTimer()
{
  if (s_timer)
    s_timer->stop();
  s_timer = std::make_unique<Timer>(500, this);
  s_timer->start();
}

void Entry::stopTimer()
{
  if (s_timer) {
    s_timer->stop();
    s_timer.reset();
  }
}

} // namespace ui
