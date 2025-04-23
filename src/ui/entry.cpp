// Aseprite UI Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/entry.h"

#include "base/string.h"
#include "os/system.h"
#include "text/draw_text.h"
#include "text/font.h"
#include "ui/clipboard_delegate.h"
#include "ui/display.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/timer.h"
#include "ui/translation_delegate.h"
#include "ui/widget.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <memory>

namespace ui {

// Shared timer between all entries.
static std::unique_ptr<Timer> s_timer;

static inline bool is_word_char(int ch)
{
  return (ch && !std::isspace(ch) && !std::ispunct(ch));
}

Entry::Entry(const int maxsize, const char* format, ...)
  : Widget(kEntryWidget)
  , m_maxsize(maxsize)
  , m_caret(0)
  , m_scroll(0)
  , m_select(0)
  , m_hidden(false)
  , m_state(false)
  , m_readonly(false)
  , m_recent_focused(false)
  , m_lock_selection(false)
  , m_persist_selection(false)
  , m_translate_dead_keys(true)
  , m_scale(1.0f, 1.0f)
{
  enableFlags(CTRL_RIGHT_CLICK);

  // formatted string
  char buf[4096]; // TODO buffer overflow
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

  // TODO support for text alignment and multi-line
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
  return int(m_boxes.size() - 1);
}

void Entry::setCaretPos(const int pos)
{
  gfx::Size caretSize = theme()->getEntryCaretSize(this);
  int textlen = lastCaretPos();
  m_caret = std::clamp(pos, 0, textlen);
  m_scroll = std::clamp(m_scroll, 0, textlen);

  // Backward scroll
  if (m_caret < m_scroll) {
    m_scroll = m_caret;
  }
  // Forward scroll
  else if (m_caret > m_scroll) {
    const gfx::Rect bounds = getEntryTextBounds();
    const int xLimit = bounds.x2();

    while (m_caret > m_scroll) {
      const float visibleTextWidth = m_boxes[m_caret].x - m_boxes[m_scroll].x;

      const int x = bounds.x + visibleTextWidth * m_scale.x + caretSize.w;

      if (x < xLimit)
        break;
      else
        ++m_scroll;
    }
  }

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
  setCaretPos((to >= 0) ? to : end + to + 1);

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
                         m_boxes[range.to - 1].to - m_boxes[range.from].from);
  else
    return std::string();
}

Entry::Range Entry::selectedRange() const
{
  Range range;
  if ((m_select >= 0) && (m_caret != m_select)) {
    range.from = std::min(m_caret, m_select);
    range.to = std::max(m_caret, m_select);

    range.from = std::clamp(range.from, 0, std::max(0, int(m_boxes.size()) - 1));
    range.to = std::clamp(range.to, 0, int(m_boxes.size()));
  }
  return range;
}

void Entry::setSuffix(const std::string& suffix)
{
  // No-op cases
  if ((!m_suffix && suffix.empty()) || (m_suffix && *m_suffix == suffix))
    return;

  m_suffix = std::make_unique<std::string>(suffix);
  invalidate();
}

std::string Entry::getSuffix()
{
  return (m_suffix ? *m_suffix : std::string());
}

void Entry::setTranslateDeadKeys(bool state)
{
  m_translate_dead_keys = state;
}

void Entry::getEntryThemeInfo(int* scroll, int* caret, int* state, Range* range) const
{
  if (scroll)
    *scroll = m_scroll;
  if (caret)
    *caret = m_caret;
  if (state)
    *state = !m_hidden && m_state;
  if (range)
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
    return gfx::Rect(m_boxes[i].x, 0, m_boxes[i].width, textHeight());
  else
    return gfx::Rect();
}

bool Entry::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kTimerMessage:
      if (hasFocus() && static_cast<TimerMessage*>(msg)->timer() == s_timer.get()) {
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

      if (m_lock_selection) {
        m_lock_selection = false;
      }
      else {
        if (!m_persist_selection)
          selectAllText();
        m_recent_focused = true;
      }

      // Start processing dead keys
      if (m_translate_dead_keys)
        os::System::instance()->setTranslateDeadKeys(true);
      break;

    case kFocusLeaveMessage:
      invalidate();

      stopTimer();

      if (!m_lock_selection && !m_persist_selection)
        deselectText();

      m_recent_focused = false;

      // Stop processing dead keys
      if (m_translate_dead_keys)
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

          case kKeyRight:
            if (msg->ctrlPressed() || msg->altPressed())
              cmd = EntryCmd::ForwardWord;
            else if (msg->cmdPressed())
              cmd = EntryCmd::EndOfLine;
            else
              cmd = EntryCmd::ForwardChar;
            break;

          case kKeyHome: cmd = EntryCmd::BeginningOfLine; break;

          case kKeyEnd:  cmd = EntryCmd::EndOfLine; break;

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
            executeCmd(EntryCmd::InsertChar,
                       keymsg->unicodeChar(),
                       (msg->shiftPressed()) ? true : false);

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
            break; // Propagate to manager
          }
        }

        executeCmd(cmd, keymsg->unicodeChar(), (msg->shiftPressed()) ? true : false);
        return true;
      }
      break;

    case kMouseDownMessage:
      captureMouse();

      // Disable selecting words if we click again (only
      // double-clicking enables selecting words again).
      if (!m_selecting_words.isEmpty())
        m_selecting_words.reset();

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
          if (m_recent_focused) {
            m_recent_focused = false;
            m_select = m_caret;
          }
          // Deselect
          else if (msg->type() == kMouseDownMessage) {
            m_select = m_caret;
          }
          // Continue selecting words
          else if (!m_selecting_words.isEmpty()) {
            Range toWord = wordRange(m_caret);
            if (toWord.from < m_selecting_words.from) {
              m_select = std::max(m_selecting_words.to, toWord.to);
              setCaretPos(std::min(m_selecting_words.from, toWord.from));
            }
            else {
              m_select = std::min(m_selecting_words.from, toWord.from);
              setCaretPos(std::max(m_selecting_words.to, toWord.to));
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

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        if (!m_selecting_words.isEmpty())
          m_selecting_words.reset();

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
      if (!hasFocus())
        requestFocus();

      m_selecting_words = wordRange(m_caret);
      selectText(m_selecting_words.from, m_selecting_words.to);

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
gfx::Size Entry::sizeHintWithText(Entry* entry, const std::string& text)
{
  const auto& font = entry->font();

  int w = font->textLength(text) + +2 * entry->theme()->getEntryCaretSize(entry).w +
          entry->border().width();

  w = std::min(w, entry->display()->workareaSizeUIScale().w / 2);

  const int h = font->lineHeight() + entry->border().height();

  return gfx::Size(w, h);
}

void Entry::onSizeHint(SizeHintEvent& ev)
{
  const auto& font = this->font();

  int trailing = font->textLength(getSuffix());
  trailing = std::max(trailing, 2 * theme()->getEntryCaretSize(this).w);

  int w = font->textLength("w") * std::min(m_maxsize, 6) + +trailing + border().width();

  w = std::min(w, display()->workareaSizeUIScale().w / 2);

  int h = font->lineHeight() + border().height();

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
  bounds.y += guiscaled_center(0, bounds.h, textHeight());
  bounds.w -= border().width();
  bounds.h = textHeight();
  return bounds;
}

int Entry::getCaretFromMouse(MouseMessage* mouseMsg)
{
  const gfx::Rect bounds = getEntryTextBounds().offset(this->bounds().origin());
  const int mouseX = mouseMsg->position().x;

  if (mouseX < bounds.x + border().left()) {
    // Scroll to the left
    return std::max(0, m_scroll - 1);
  }

  int lastPos = lastCaretPos();
  int scroll = m_scroll;
  int i = std::min(scroll, lastPos);
  int scrollX = m_boxes[scroll].x;
  for (; i < lastPos; ++i) {
    const int x = bounds.x + m_boxes[i].x * m_scale.x - scrollX;

    if (mouseX >= x && mouseX < x + m_boxes[i].width)
      break;

    if (mouseX > bounds.x2()) {
      if (x >= bounds.x2()) {
        // Scroll to the right
        i = std::min(++scroll, lastPos);
        if (i == lastPos)
          break;

        scrollX = m_boxes[scroll].x;
      }
    }
    else if (x > mouseX)
      break;
  }

  return std::clamp(i, 0, lastPos);
}

void Entry::executeCmd(const EntryCmd cmd,
                       const base::codepoint_t unicodeChar,
                       const bool shift_pressed)
{
  std::string text = this->text();
  const Range range = selectedRange();

  switch (cmd) {
    case EntryCmd::NoOp: break;

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

        const std::string unicodeStr = base::codepoint_to_utf8(unicodeChar);

        text.insert(m_boxes[m_caret].from, unicodeStr);
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
          text.erase(m_boxes[m_caret].from, m_boxes[m_caret].to - m_boxes[m_caret].from);
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
          text.erase(m_boxes[m_maxsize].from, text.size() - m_boxes[m_maxsize].from);
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
          --m_caret;
          text.erase(m_boxes[m_caret].from, m_boxes[m_caret].to - m_boxes[m_caret].from);
        }
      }

      m_select = -1;
      break;

    case EntryCmd::DeleteBackwardWord:
      m_select = m_caret;
      backwardWord();
      if (m_caret < m_select) {
        text.erase(m_boxes[m_caret].from, m_boxes[m_select - 1].to - m_boxes[m_caret].from);
      }
      m_select = -1;
      break;

    case EntryCmd::DeleteForwardToEndOfLine: text.erase(m_boxes[m_caret].from); break;

    case EntryCmd::SelectAll:                selectAllText(); break;
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
    for (; i >= 0; --i) {
      if (!is_word_char(m_boxes[i].codepoint))
        break;
    }
    ++i;
    for (; j <= last; ++j) {
      if (!is_word_char(m_boxes[j].codepoint))
        break;
    }
  }
  // Select punctuation space
  else {
    for (; i >= 0; --i) {
      if (is_word_char(m_boxes[i].codepoint))
        break;
    }
    ++i;
    for (; j <= last; ++j) {
      if (is_word_char(m_boxes[j].codepoint))
        break;
    }
  }
  return Range(i, j);
}

bool Entry::isPosInSelection(int pos)
{
  return (pos >= std::min(m_caret, m_select) && pos <= std::max(m_caret, m_select));
}

void Entry::showEditPopupMenu(const gfx::Point& pt)
{
  Menu menu;

  auto* clipboard = UISystem::instance()->clipboardDelegate();
  if (!clipboard)
    return;

  auto* translate = UISystem::instance()->translationDelegate();
  ASSERT(translate); // We provide UISystem as default translation delegate
  if (!translate)
    return;

  MenuItem cut(translate->cut());
  MenuItem copy(translate->copy());
  MenuItem paste(translate->paste());
  MenuItem selectAll(translate->selectAll());
  menu.addChild(&cut);
  menu.addChild(&copy);
  menu.addChild(&paste);
  menu.addChild(new MenuSeparator);
  menu.addChild(&selectAll);

  for (auto* item : menu.children())
    item->processMnemonicFromText();

  cut.Click.connect([this] { executeCmd(EntryCmd::Cut, 0, false); });
  copy.Click.connect([this] { executeCmd(EntryCmd::Copy, 0, false); });
  paste.Click.connect([this] { executeCmd(EntryCmd::Paste, 0, false); });
  selectAll.Click.connect([this] { executeCmd(EntryCmd::SelectAll, 0, false); });

  copy.setEnabled(m_select >= 0);
  cut.setEnabled(m_select >= 0 && !isReadOnly());
  paste.setEnabled(clipboard->hasClipboardText() && !isReadOnly());

  menu.showPopup(pt, display());
}

class Entry::CalcBoxesTextDelegate : public text::DrawTextDelegate {
public:
  CalcBoxesTextDelegate(const int end) : m_end(end) {}

  const Entry::CharBoxes& boxes() const { return m_boxes; }

  void preProcessChar(const int index,
                      const base::codepoint_t codepoint,
                      gfx::Color& fg,
                      gfx::Color& bg,
                      const gfx::Rect& charBounds) override
  {
    if (!m_boxes.empty())
      m_boxes.back().to = index;

    m_box = CharBox();
    m_box.codepoint = codepoint;
    m_box.from = index;
    m_box.to = m_end;
  }

  bool preDrawChar(const gfx::Rect& charBounds) override
  {
    m_box.x = charBounds.x;
    m_box.width = charBounds.w;
    return true;
  }

  void postDrawChar(const gfx::Rect& charBounds) override { m_boxes.push_back(m_box); }

private:
  Entry::CharBox m_box;
  Entry::CharBoxes m_boxes;
  int m_end;
};

void Entry::recalcCharBoxes(const std::string& text)
{
  int lastTextIndex = int(text.size());
  CalcBoxesTextDelegate delegate(lastTextIndex);
  float lastX = text::draw_text(nullptr,
                                theme()->fontMgr(),
                                font(),
                                text,
                                gfx::ColorNone,
                                gfx::ColorNone,
                                0,
                                0,
                                &delegate,
                                onGetTextShaperFeatures())
                  .w;
  m_boxes = delegate.boxes();

  if (!m_boxes.empty()) {
    m_boxes.back().to = lastTextIndex;
  }

  // A last box for the last position
  CharBox box;
  box.codepoint = 0;
  box.from = box.to = lastTextIndex;
  box.x = lastX;
  box.width = theme()->getEntryCaretSize(this).w;
  m_boxes.push_back(box);
}

bool Entry::shouldStartTimer(bool hasFocus)
{
  return (!m_hidden && hasFocus && isEnabled());
}

void Entry::deleteRange(const Range& range, std::string& text)
{
  text.erase(m_boxes[range.from].from, m_boxes[range.to - 1].to - m_boxes[range.from].from);
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
