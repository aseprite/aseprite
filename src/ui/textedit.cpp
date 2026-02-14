// Aseprite
// Copyright (C) 2024-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/textedit.h"

#include "base/replace_string.h"
#include "base/split_string.h"
#include "os/system.h"
#include "text/font_metrics.h"
#include "text/font_mgr.h"
#include "text/text_blob.h"
#include "ui/display.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/timer.h"
#include "ui/view.h"

#include <algorithm>
#include <limits>

namespace ui {

// Shared timer between all editors.
static std::unique_ptr<Timer> s_timer;

TextEdit::TextEdit() : Widget(kTextEditWidget), m_caret(&m_lines)
{
  enableFlags(CTRL_RIGHT_CLICK);
  setFocusStop(true);
  initTheme();
}

void TextEdit::cut()
{
  if (m_selection.isEmpty())
    return;

  copy();

  deleteSelection();
}

void TextEdit::copy()
{
  if (m_selection.isEmpty())
    return;

  set_clipboard_text(selectedText());
}

void TextEdit::paste()
{
  std::string clipboard;
  if (!get_clipboard_text(clipboard) || clipboard.empty())
    return;

  deleteSelection();
  base::replace_string(clipboard, "\r\n", "\n");

  if (insertText(clipboard)) {
    updateViewSize();
    Change();
    ensureCaretVisible();
  }
}

void TextEdit::selectAll()
{
  if (m_textContent.empty() || m_lines.empty())
    return;

  const Caret startCaret(&m_lines);
  Caret endCaret(&m_lines);
  endCaret.set(m_lines.size() - 1, m_lines.back().glyphCount);

  m_selection.set(startCaret, endCaret);
}

bool TextEdit::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kTimerMessage: {
      if (hasFocus() && static_cast<TimerMessage*>(msg)->timer() == s_timer.get()) {
        m_drawCaret = !m_drawCaret;
        invalidateRect(m_caretRect);
      }
      break;
    }
    case kFocusEnterMessage: {
      startTimer();
      m_drawCaret = true;
      invalidate();
      updateCaretPosOnScreen();
      break;
    }
    case kFocusLeaveMessage: {
      stopTimer();
      m_drawCaret = false;
      invalidateRect(m_caretRect);
      updateCaretPosOnScreen();
      break;
    }
    case kKeyDownMessage: {
      if (hasFocus() && onKeyDown(static_cast<KeyMessage*>(msg))) {
        m_drawCaret = true;
        invalidate();
        ensureCaretVisible();
        return true;
      }
      break;
    }
    case kDoubleClickMessage: {
      if (!hasFocus())
        requestFocus();

      const auto* mouseMessage = static_cast<MouseMessage*>(msg);
      Caret caret = caretFromPosition(mouseMessage->position());
      if (!caret.isValid())
        return false;

      m_selection = m_selectionWords = Selection::SelectWords(caret);
      m_caret = m_selection.end();
      invalidate();
      captureMouse();
      return true;
    }
    case kMouseDownMessage:
      if (msg->shiftPressed())
        m_lockedSelectionStart = m_selection.isEmpty() ? m_caret : m_selection.start();
      else if (!hasCapture() && static_cast<MouseMessage*>(msg)->left()) {
        // Only clear the selection when we don't have a capture, to avoid stepping on double click
        // selection.
        m_selection.clear();
      }

      captureMouse();

      stopTimer();
      m_drawCaret = true;

      [[fallthrough]]; // onMouseMove sets our caret position when we click.
    case kMouseMoveMessage:
      if (hasCapture() && onMouseMove(static_cast<MouseMessage*>(msg))) {
        invalidate();
        ensureCaretVisible();
        return true;
      }
      break;
    case kMouseUpMessage: {
      if (hasCapture()) {
        releaseMouse();
        startTimer();

        if (!m_selectionWords.isEmpty())
          m_selectionWords.clear();

        const auto* mouseMsg = static_cast<MouseMessage*>(msg);
        if (mouseMsg->right()) {
          showEditPopupMenu(display(), mouseMsg->position());
          requestFocus();
          return true;
        }

        if (msg->shiftPressed()) {
          m_selection.set(m_lockedSelectionStart, m_caret);
        }
        m_lockedSelectionStart.clear();
      }
      break;
    }
    case kMouseWheelMessage: {
      View::scrollByMessage(this, msg);
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

bool TextEdit::onKeyDown(const KeyMessage* keymsg)
{
  const Cmd cmd = cmdFromKeyMessage(keymsg);
  if (cmd != Cmd::NoOp) {
    executeCmd(cmd, keymsg->unicodeChar(), keymsg->shiftPressed());
    return true;
  }

  const KeyScancode scancode = keymsg->scancode();
  if (scancode == kKeyEnter || scancode == kKeyEnterPad) {
    deleteSelection();
    insertText("\n");
    updateViewSize();
    ensureCaretVisible();
    return true;
  }

  if (keymsg->unicodeChar() >= 32) {
    deleteSelection();
    if (keymsg->isDeadKey()) {
      return true;
    }

    executeCmd(Cmd::InsertChar, keymsg->unicodeChar());
    return true;
  }

  // Consume all key down of modifiers only, e.g. so the user can
  // press first "Ctrl" key, and then "Ctrl+C" combination.
  if (scancode >= kKeyFirstModifierScancode)
    return true;

  return false;
}

bool TextEdit::onMouseMove(const MouseMessage* mouseMessage)
{
  const Caret mouseCaret = caretFromPosition(mouseMessage->position());
  if (!mouseCaret.isValid() || mouseMessage->right())
    return false;

  m_caret = mouseCaret;

  if (!m_lockedSelectionStart.isValid()) {
    m_lockedSelectionStart = m_caret;
    return true;
  }

  if (!m_selectionWords.isEmpty()) {
    m_selection = m_selectionWords;
    m_selection |= Selection::SelectWords(m_caret);
    if (m_caret < m_selectionWords.start())
      m_caret = m_selection.start();
    else
      m_caret = m_selection.end();
  }
  else
    m_selection.set(m_lockedSelectionStart, m_caret);

  return true;
}

void TextEdit::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  const auto* view = View::getView(this);
  ASSERT(view);
  if (!view)
    return;

  const gfx::Rect rect = view->viewportBounds().offset(-bounds().origin());
  g->drawRect(rect, m_colors.background);

  gfx::PointF point(clientChildrenBounds().origin());
  const gfx::Rect clipBounds = g->getClipBounds();

  for (const auto& line : m_lines) {
    const bool caretLine = (line.i == m_caret.line());

    // Skip drawing lines when they're out of scroll bounds or they're outside the clip bounds,
    // unless we're in the caret line, in which case we need to draw the text to avoid blank
    // characters.
    const bool skip =
      (!clipBounds.intersects(gfx::Rect(point.x, point.y, line.width, line.height)) && !caretLine);

    if (!skip) {
      g->drawTextBlob(line.blob, point, m_colors.text);

      // Drawing the selection rect and any selected text.
      // We're technically drawing over the old text, so ideally we want to clip that off as well?
      const gfx::RectF selectionRect = getSelectionRect(line, point);
      if (!selectionRect.isEmpty()) {
        g->drawRect(selectionRect, m_colors.selectedBackground);

        const IntersectClip clip(g, selectionRect);
        if (clip)
          g->drawTextBlob(line.blob, point, m_colors.selectedText);
      }
    }

    point.y += line.height;
  }

  m_caretRect = caretBounds();
  if (m_drawCaret && !m_caretRect.isEmpty())
    g->drawRect(m_caretRect, m_colors.text);
  m_caretRect.offset(origin());
}

void TextEdit::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  m_colors = theme()->getTextColors(this);

  // Invalidate all blobs
  onSetText();
}

void TextEdit::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(m_textSize + border());
}

void TextEdit::onExecuteCmd(const Cmd cmd,
                            const base::codepoint_t unicodeChar,
                            const bool expandSelection)
{
  const Caret prevCaret = m_caret;

  switch (cmd) {
    case Cmd::NoOp:       break;

    case Cmd::InsertChar: insertCharacter(unicodeChar); break;

    case Cmd::PrevChar:   m_caret.left(); break;
    case Cmd::PrevWord:   m_caret.leftWord(); break;
    case Cmd::PrevLine:   m_caret.up(); break;

    case Cmd::NextChar:   m_caret.right(); break;
    case Cmd::NextWord:   m_caret.rightWord(); break;
    case Cmd::NextLine:   m_caret.down(); break;

    case Cmd::BegOfLine:  m_caret.setPos(0); break;
    case Cmd::EndOfLine:  m_caret.eol(); break;

    case Cmd::BegOfFile:  m_caret.set(0, 0); break;
    case Cmd::EndOfFile:  m_caret.set(m_lines.back().i, m_lines.back().glyphCount); break;

    case Cmd::DeletePrevChar:
    case Cmd::DeleteNextChar:
    case Cmd::DeletePrevWord:
    case Cmd::DeleteNextWord:
    case Cmd::DeleteToEndOfLine:
      if (m_selection.isEmpty() || !m_selection.isValid()) {
        Caret startCaret = m_caret;
        Caret endCaret = m_caret;
        switch (cmd) {
          case Cmd::DeletePrevChar:    startCaret.left(); break;
          case Cmd::DeletePrevWord:    startCaret.leftWord(); break;
          case Cmd::DeleteNextChar:    endCaret.right(); break;
          case Cmd::DeleteNextWord:    endCaret.rightWord(); break;
          case Cmd::DeleteToEndOfLine: endCaret.eol(); break;
        }
        m_selection.set(startCaret, endCaret);
      }
      deleteSelection();
      return;

    case Cmd::Cut:       cut(); return;
    case Cmd::Copy:      copy(); return;
    case Cmd::Paste:     paste(); return;
    case Cmd::SelectAll: selectAll(); return;
  }

  // Selection modification
  if (expandSelection) {
    if (!m_selection.isValid() || m_selection.isEmpty()) {
      m_lockedSelectionStart = prevCaret;
    }

    m_selection.set(m_lockedSelectionStart, m_caret);
  }
  else
    m_selection.clear();
}

gfx::Rect TextEdit::caretBounds() const
{
  const Line& line = m_caret.lineObj();

  gfx::Point origin = clientChildrenBounds().origin();
  gfx::Rect rc(origin, theme()->getCaretSize(const_cast<TextEdit*>(this)));

  if (m_caret.inEol()) {
    rc.x += line.width;
  }
  else if (m_caret.pos() > 0) {
    rc.x += line.getBounds(m_caret.pos()).x;
  }

  gfx::PointF point(origin);
  for (const auto& l : m_lines) {
    if (l.i >= m_caret.line())
      break;
    point.y += l.height;
  }

  rc.h = std::max(rc.h, line.height);
  rc.y = point.y + line.height / 2 - rc.h / 2;
  return rc;
}

gfx::Point TextEdit::caretPosOnScreen() const
{
  const os::Window* nativeWindow = display()->nativeWindow();
  if (!nativeWindow)
    return {};
  return nativeWindow->pointToScreen(caretBounds().point2() + bounds().origin());
}

void TextEdit::updateCaretPosOnScreen()
{
  if (hasFocus())
    os::System::instance()->setTextInput(true, caretPosOnScreen());
  else
    os::System::instance()->setTextInput(false);
}

gfx::RectF TextEdit::getSelectionRect(const Line& line, const gfx::PointF& offset) const
{
  if (m_selection.isEmpty() || !m_selection.isValid())
    return gfx::RectF();

  if (m_selection.start().line() > line.i || m_selection.end().line() < line.i)
    return gfx::RectF();

  gfx::RectF selectionRect(offset, gfx::SizeF{});

  if (!line.blob) {
    // No blob so this must be an empty line in the middle of a selection, just give it a marginal
    // width so it's noticeable.
    selectionRect.w = line.height / 2.0;
  }
  else if (
    // Detect when this entire line is selected, to avoid doing any runs and just painting it all
    // Case 1: Start and end line is this line, and the firstPos and endPos is 0 and the line's
    // length.
    (m_selection.start().line() == line.i && m_selection.end().line() == line.i &&
     m_selection.start().pos() == 0 && m_selection.end().pos() == line.glyphCount)
    // Case 2: We start at this line and position zero, we end in a higher line.
    || (m_selection.start().line() == line.i && m_selection.start().pos() == 0 &&
        m_selection.end().line() > line.i)
    // Case 3: We started on a previous line, and we continue on another.
    || (m_selection.start().line() < line.i && m_selection.end().line() > line.i)) {
    selectionRect.w = line.width;
  }
  else if (m_selection.start().line() < line.i && m_selection.end().line() == line.i) {
    // The selection ends in this line, starts from the leftmost side
    const auto& lineBounds = line.getBounds(0, m_selection.end().pos());
    selectionRect.x += lineBounds.x;
    selectionRect.w = lineBounds.w;
  }
  else if (m_selection.start().line() == line.i && m_selection.end().line() == line.i) {
    // Selection is contained within this line
    const auto& lineBounds = line.getBounds(m_selection.start().pos(), m_selection.end().pos());
    selectionRect.x += lineBounds.x;
    selectionRect.w = lineBounds.w;
  }
  else if (m_selection.start().line() == line.i) {
    // The selection starts in this line at an offset position, and ends at the end of the run
    const auto& lineBounds = line.getBounds(m_selection.start().pos(),
                                            m_selection.start().lineObj().glyphCount);
    selectionRect.x += lineBounds.x;
    selectionRect.w = lineBounds.w;
  }

  selectionRect.h = line.height; // Normalize the height of the rect so it doesn't vary.
  return selectionRect;
}

TextEdit::Caret TextEdit::caretFromPosition(const gfx::Point& position)
{
  const auto* view = View::getView(this);
  if (!view)
    return Caret();

  if (m_lines.empty())
    return Caret(&m_lines, 0, 0);

  // Deduce the position the user wants to go when clicking outside of bounds
  if (!view->viewportBounds().contains(position)) {
    if (position.y < view->viewportBounds().y) {
      return Caret(&m_lines, 0, 0);
    }

    if (position.y > view->viewportBounds().y2()) {
      return Caret(&m_lines, m_lines.size() - 1, m_lines.back().glyphCount);
    }

    if (position.x > view->viewportBounds().x2()) {
      Caret caret = m_caret;
      caret.right();
      return caret;
    }

    return Caret();
  }

  // Normalize the mouse position to the internal coordinates of the widget
  gfx::PointF offsetPosition(position - childrenBounds().origin());
  Caret caret(&m_lines);

  // First check if the offset position is blank (below all the lines)
  if (offsetPosition.y >= maxHeight()) {
    // Get the last character in the last line.
    caret.setLine(m_lines.size() - 1);

    // Check the line width and if we're more than halfway past the line, we can set the caret to
    // the full line.
    caret.setPos((offsetPosition.x > caret.lineObj().width / 2) ? caret.lineObj().glyphCount : 0);
    return caret;
  }

  int lineStartY = 0;
  for (const Line& line : m_lines) {
    const int lineEndY = lineStartY + line.height;
    if (offsetPosition.y < lineStartY || offsetPosition.y >= lineEndY) {
      lineStartY = lineEndY;
      continue; // We're not in this line
    }

    caret.setLine(line.i);

    if (!line.blob)
      break; // Line has no text, we can end it here.

    if (offsetPosition.x > line.width) {
      // Clicking on the blank space next to a line should put our caret at the end of it.
      caret.setPos(line.glyphCount);
      break;
    }

    // Find the exact character we're standing on, with a slight bias to the left or right
    // depending on where we click wrt the glyph bounds
    int advance = 0;
    int best = 0;
    float bestDiff = std::numeric_limits<float>::max();

    line.blob->visitRuns([&](const text::TextBlob::RunInfo& run) {
      for (int i = 0; i < run.glyphCount; ++i) {
        const float diff = std::fabs(run.getGlyphBounds(i).center().x - offsetPosition.x);
        if (diff < bestDiff) {
          best = advance;
          bestDiff = diff;
        }
        ++advance;
      }
    });

    // Done, we've found the clicked caret position
    caret.setPos(best);
    break;
  }

  return caret;
}

void TextEdit::insertCharacter(base::codepoint_t character)
{
  const std::string unicodeStr = base::codepoint_to_utf8(character);

  if (m_lines.empty()) {
    // Fast path for the first char.
    m_textContent = unicodeStr;
    rebuildLines();
    m_caret.setPos(m_caret.pos() + 1);
    return;
  }

  // Get the absolute position before modifying the line
  // text/glyphs/UTF-8 information. This position must be used to
  // modify the whole Widget::text()
  const int absPos = m_caret.absolutePos();

  Line& line = m_caret.lineObj();
  const int oldglyphs = line.glyphCount;
  line.insertText(m_caret.pos(), unicodeStr);
  buildLineBlob(m_caret.line());
  const int delta = line.glyphCount - oldglyphs;

  m_textContent.insert(absPos, unicodeStr);
  m_caret.setPos(m_caret.pos() + delta);
  updateCaretPosOnScreen();
  updateViewSize();
  onTextChanged();
}

void TextEdit::deleteSelection()
{
  if (m_selection.isEmpty() || !m_selection.isValid())
    return;

  m_textContent.erase(m_textContent.begin() + m_selection.start().absolutePos(),
                      m_textContent.begin() + m_selection.end().absolutePos());

  if (m_selection.start().line() == m_selection.end().line()) {
    Line& line = m_selection.start().lineObj();
    int end;
    if (m_selection.end().inEol())
      end = line.utfSize.back().end;
    else
      end = line.utfSize[m_selection.end().pos()].begin;

    line.text.erase(line.text.begin() + line.utfSize[m_selection.start().pos()].begin,
                    line.text.begin() + end);
    // Only rebuilds the one line
    buildLineBlob(m_selection.start().line());
  }
  else {
    rebuildLines();
  }

  m_caret = m_selection.start();
  m_selection.clear();
  updateCaretPosOnScreen();
  updateViewSize();
  onTextChanged();
}

void TextEdit::ensureCaretVisible()
{
  auto* view = View::getView(this);
  if (!view || !m_caret.isValid())
    return;

  updateViewSize();
  if (!view->hasScrollBars())
    return;

  gfx::Point scroll = view->viewScroll();
  gfx::Rect vp = view->viewportBounds();
  gfx::Rect caret = caretBounds();

  // Try to show the top or left paddings when we are in line or position 0
  if (m_caret.line() == 0)
    caret.y = 0;
  if (m_caret.pos() == 0)
    caret.x = 0;

  caret.offset(origin());

  if (caret.x < vp.x)
    scroll.x = caret.x - bounds().x;
  else if (caret.x > vp.x + vp.w - caret.w)
    scroll.x = (caret.x - bounds().x - vp.w + caret.w);

  if (caret.y < vp.y)
    scroll.y = caret.y - bounds().y;
  else if (caret.y > vp.y + vp.h - caret.h)
    scroll.y = (caret.y - bounds().y - vp.h + caret.h);

  view->setViewScroll(scroll);
  updateCaretPosOnScreen();
}

void TextEdit::onSetText()
{
  setTextContent(text());
  updateViewSize();
  Widget::onSetText();
}

void TextEdit::onSetFont()
{
  Widget::onSetFont();

  // Invalidate all blobs
  onSetText();
}

int TextEdit::maxHeight() const
{
  int h = 0;
  for (const auto& line : m_lines)
    h += line.height;
  return h;
}

void TextEdit::startTimer()
{
  if (s_timer)
    s_timer->stop();
  s_timer = std::make_unique<Timer>(500, this);
  s_timer->start();
}

void TextEdit::stopTimer()
{
  if (s_timer) {
    s_timer->stop();
    s_timer.reset();
  }
}

void TextEdit::updateViewSize()
{
  gfx::Size reqSize(0, 0);

  for (const auto& line : m_lines) {
    reqSize.w = std::max(reqSize.w, line.width);
    reqSize.h += line.height;
  }

  if (m_textSize != reqSize) {
    m_textSize = reqSize;
    if (auto* view = View::getView(this))
      view->updateView();
  }
}

struct Utf8RangeBuilder : public text::TextBlob::RunHandler {
  explicit Utf8RangeBuilder(int minSize) { ranges.reserve(minSize); }

  void commitRunBuffer(TextBlob::RunInfo& info) override
  {
    for (int i = 0; i < info.glyphCount; ++i) {
      ranges.push_back(info.getGlyphUtf8Range(i));
    }
  }

  std::vector<TextBlob::Utf8Range> ranges;
};

void TextEdit::setTextContent(const std::string& text)
{
  m_textContent = text;
  rebuildLines();
  onTextChanged();
}

std::string TextEdit::selectedText() const
{
  if (m_selection.isEmpty() || !m_selection.isValid())
    return std::string();

  const int startPos = m_selection.start().absolutePos();
  const int endPos = m_selection.end().absolutePos();
  return m_textContent.substr(startPos, endPos - startPos);
}

bool TextEdit::insertText(const std::string& str)
{
  if (str.empty())
    return false;

  if (!m_caret.isValid())
    m_caret = Caret(&m_lines, 0, 0);

  deleteSelection();

  std::string cleanedStr = str;
  // Replace \r\n with \n
  size_t pos = 0;
  while ((pos = cleanedStr.find("\r\n", pos)) != std::string::npos) {
    cleanedStr.replace(pos, 2, "\n");
  }

  const int targetAbsPos = m_caret.absolutePos() + cleanedStr.size();

  m_textContent.insert(m_caret.absolutePos(), cleanedStr);

  if (!m_lines.empty() && cleanedStr.find('\n') == std::string::npos) {
    // Single line insert - optimize by just rebuilding the current line
    Line& line = m_caret.lineObj();
    line.insertText(m_caret.pos(), cleanedStr);
    buildLineBlob(m_caret.line());
    onTextChanged();
    m_caret.advanceBy(cleanedStr.size());
  }
  else {
    // Multi-line insert - rebuild everything
    rebuildLines();

    // Find the new caret position from the target absolute position
    int absPos = 0;
    for (int i = 0; i < m_lines.size(); ++i) {
      const Line& line = m_lines[i];
      const int lineTextSize = line.text.size();
      const int lineEndPos = absPos + lineTextSize;

      if (targetAbsPos <= lineEndPos) {
        // Caret is on this line - convert byte offset to glyph position
        const int byteOffset = targetAbsPos - absPos;
        int glyphPos = 0;
        for (int j = 0; j < line.glyphCount; ++j) {
          if (line.utfSize[j].begin >= byteOffset)
            break;
          glyphPos = j + 1;
        }
        m_caret.set(i, glyphPos);
        break;
      }

      absPos = lineEndPos + 1; // +1 for the newline character
    }

    onTextChanged();
  }

  return true;
}

void TextEdit::Line::insertText(int pos, const std::string& str)
{
  if (pos == 0)
    text.insert(0, str);
  else if (pos == glyphCount)
    text.append(str);
  else
    text.insert(utfSize[pos].begin, str);
}

gfx::RectF TextEdit::Line::getBounds(const int glyph) const
{
  if (!blob)
    return gfx::RectF();

  int advance = 0;
  gfx::Rect result;
  blob->visitRuns([&advance, &result, glyph](const text::TextBlob::RunInfo& run) {
    for (int i = 0; i < run.glyphCount; ++i) {
      if (advance == glyph)
        result = run.getGlyphBounds(i);
      ++advance;
    }
  });
  return result;
}

gfx::RectF TextEdit::Line::getBounds(const int startGlyph, const int endGlyph) const
{
  ASSERT(endGlyph > startGlyph);
  if (startGlyph == endGlyph || !blob)
    return gfx::RectF();

  int advance = 0; // The amount of glyphs we've advanced through.
  gfx::Rect resultBounds;

  blob->visitRuns([&](text::TextBlob::RunInfo& run) {
    if (advance >= endGlyph)
      return;

    if (startGlyph > (advance + run.glyphCount)) {
      advance += run.glyphCount;
      return; // Skip this run
    }

    int j = 0;
    if (advance < startGlyph) {
      j = startGlyph - advance;
      advance += j;
    }

    for (; j < run.glyphCount; ++j) {
      ++advance;
      resultBounds |= run.getGlyphBounds(j);

      if (advance >= endGlyph)
        return;
    }
  });

  ASSERT(advance == endGlyph);

  return resultBounds;
}

void TextEdit::Caret::setPos(int pos)
{
  ASSERT(pos >= 0 && pos <= lineObj().glyphCount);
  m_pos = pos;
}

void TextEdit::Caret::set(int line, int pos)
{
  m_line = line;
  m_pos = pos;
}

bool TextEdit::Caret::left()
{
  if (--m_pos < 0) {
    if (m_line == 0) {
      m_pos = 0;
      return false;
    }
    --m_line;
    eol();
  }
  return true;
}

bool TextEdit::Caret::leftWord()
{
  auto startPos = m_pos;
  while (left()) {
    if (isWordPart())
      break;
  }
  while (isWordPart()) {
    if (!left())
      return m_pos != startPos;
  }
  right();
  return true;
}

bool TextEdit::Caret::right()
{
  if (++m_pos > lineObj().glyphCount) {
    if (m_line == int(m_lines->size()) - 1) {
      --m_pos; // Undo movement, we've reached the end of the text.
      return false;
    }
    ++m_line;
    m_pos = 0;
  }
  return true;
}

bool TextEdit::Caret::rightWord()
{
  auto startPos = m_pos;
  while (right()) {
    if (isWordPart())
      break;
  }
  while (isWordPart()) {
    if (!right())
      return m_pos != startPos;
  }
  return true;
}

void TextEdit::Caret::up()
{
  if (m_line == 0) {
    m_pos = 0;
    return;
  }
  m_line = std::clamp(m_line - 1, 0, int(m_lines->size()) - 1);
  m_pos = std::clamp(m_pos, 0, lineObj().glyphCount);
}

void TextEdit::Caret::down()
{
  if (m_line == int(m_lines->size()) - 1) {
    eol();
    return;
  }
  m_line = std::clamp(m_line + 1, 0, int(m_lines->size()) - 1);
  m_pos = std::clamp(m_pos, 0, lineObj().glyphCount);
}

void TextEdit::Caret::eol()
{
  m_pos = lineObj().glyphCount;
}

int TextEdit::Caret::absolutePos() const
{
  if (m_pos == 0 && m_line == 0)
    return 0;

  // Accumulate UTF-8 offsets from all previous lines.
  int apos = 0;
  for (int i = 0; i < m_line; ++i) {
    const Line& line = (*m_lines)[i];

    if (!line.text.empty() && !line.utfSize.empty())
      apos += line.utfSize.back().end;

    apos += 1; // Newline glyph.
  }

  const Line& line = lineObj();
  if (line.text.empty() || m_pos == 0)
    return apos;

  if (m_pos >= line.utfSize.size())
    apos += line.utfSize.back().end;
  else
    apos += line.utfSize[m_pos].begin;

  return apos;
}

bool TextEdit::Caret::isWordPart() const
{
  if (!lineObj().glyphCount || lineObj().utfSize.size() <= m_pos)
    return false;

  const auto& utfPos = lineObj().utfSize[m_pos];
  const std::string_view word = textView().substr(utfPos.begin, utfPos.end - utfPos.begin);
  return (!word.empty() && std::isspace(word[0]) == 0 && std::ispunct(word[0]) == 0);
}

void TextEdit::Caret::advanceBy(int characters)
{
  int remaining = characters;
  int activeLine = m_line;
  for (int i = m_line; i < m_lines->size(); ++i) {
    const auto& line = (*m_lines)[i];
    for (int j = m_pos; j < line.glyphCount; ++j) {
      remaining -= line.utfSize[j].end - line.utfSize[j].begin;
      right();

      if (remaining <= 0)
        return;

      if (m_line != activeLine) {
        activeLine = m_line;
        break;
      }
    }
  }
}

bool TextEdit::Caret::isValid() const
{
  if (m_lines == nullptr)
    return false;

  if (m_line >= m_lines->size())
    return false;

  if (m_pos > lineObj().glyphCount)
    return false;

  return true;
}

void TextEdit::Caret::clear()
{
  m_lines = nullptr;
  m_line = 0;
  m_pos = 0;
}

TextEdit::Selection TextEdit::Selection::SelectWords(const Caret& from)
{
  Caret left = from;
  Caret right = from;

  // Select word space
  if (left.isWordPart()) {
    while (left.left()) {
      if (!left.isWordPart()) {
        left.right();
        break;
      }
    }
    while (right.right()) {
      if (!right.isWordPart())
        break;
    }
  }
  // Select punctuation space
  else {
    while (left.left()) {
      if (left.isWordPart()) {
        left.right();
        break;
      }
    }
    while (right.right()) {
      if (right.isWordPart())
        break;
    }
  }

  return Selection(left, right);
}

void TextEdit::Selection::set(const Caret& startCaret, const Caret& endCaret)
{
  if (endCaret < startCaret) {
    m_start = endCaret;
    m_end = startCaret;
  }
  else {
    m_start = startCaret;
    m_end = endCaret;
  }
}

void TextEdit::Selection::clear()
{
  m_start.clear();
  m_end.clear();
}

void TextEdit::rebuildLines()
{
  std::vector<std::string_view> newLines;
  newLines.reserve(m_lines.size());
  m_lines.clear();
  base::split_string(m_textContent, newLines, "\n");
  m_lines.reserve(newLines.size());

  for (const auto& lineString : newLines) {
    Line newLine;
    newLine.text = std::string(lineString);
    newLine.i = m_lines.size();
    m_lines.push_back(newLine);
  }

  // At least one line
  if (m_lines.empty()) {
    Line emptyLine;
    emptyLine.i = 0;
    m_lines.push_back(emptyLine);
  }

  for (int i = 0; i < m_lines.size(); ++i)
    buildLineBlob(i);

  // Keep the caret in a valid position
  if (!m_caret.isValid()) {
    const int line = std::clamp(m_caret.line(), 0, int(m_lines.size()) - 1);
    m_caret = Caret(&m_lines, line, std::clamp(m_caret.pos(), 0, m_lines[line].glyphCount));
  }

  updateCaretPosOnScreen();
}

void TextEdit::buildLineBlob(int lineIndex)
{
  Line& line = m_lines[lineIndex];
  line.utfSize.clear();

  if (line.text.empty()) {
    line.blob = nullptr;
    line.width = 0;
    line.glyphCount = 0;
    line.height = font()->metrics(nullptr);
    return;
  }

  Utf8RangeBuilder rangeBuilder(line.text.size());
  line.blob = text::TextBlob::MakeWithShaper(theme()->fontMgr(), font(), line.text, &rangeBuilder);

  line.utfSize = std::move(rangeBuilder.ranges);
  line.glyphCount = line.utfSize.size();

  line.width = line.blob->bounds().w;
  line.height = std::max(int(line.blob->bounds().h), int(font()->metrics(nullptr)));
}

void TextEdit::onTextChanged()
{
  // Sync Widget's m_text with our m_textContent
  setTextQuiet(m_textContent);
  Change();
}

} // namespace ui
