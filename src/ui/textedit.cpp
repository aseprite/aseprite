// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
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
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/scroll_helper.h"
#include "ui/scroll_region_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/timer.h"
#include "ui/view.h"
#include <algorithm>

namespace ui {

// Shared timer between all editors.
static std::unique_ptr<Timer> s_timer;

TextEdit::TextEdit() : Widget(kGenericWidget), m_caret(&m_lines)
{
  enableFlags(CTRL_RIGHT_CLICK);
  setFocusStop(true);
  InitTheme.connect([this] {
    setBorder(gfx::Border(2) * guiscale()); // TODO: Move to theme
  });
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

  const size_t startPos = m_selection.start().absolutePos();
  set_clipboard_text(text().substr(startPos, m_selection.end().absolutePos() - startPos));
}

void TextEdit::paste()
{
  if (!m_caret.isValid())
    m_caret = Caret(&m_lines, 0, 0); // TODO: Can we just ensure this doesn't happen?

  std::string clipboard;
  if (!get_clipboard_text(clipboard) || clipboard.empty())
    return;

  deleteSelection();

#if LAF_WINDOWS
  base::replace_string(clipboard, "\r\n", "\n");
#endif

  std::string newText = text();
  newText.insert(m_caret.absolutePos(), clipboard);

  if (!m_lines.empty() && clipboard.find('\n') == std::string::npos) {
    auto& line = m_lines[m_caret.line()];
    line.insertText(m_caret.pos(), clipboard);
    line.buildBlob(this);
    setTextQuiet(newText);
    Change();
  }
  else {
    setText(newText);
  }

  m_caret.advanceBy(clipboard.size());
}

void TextEdit::selectAll()
{
  if (text().empty() || m_lines.empty())
    return;

  const Caret startCaret(&m_lines);
  Caret endCaret(startCaret);
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
      m_drawCaret = true; // Immediately draw the caret for fast UI feedback.
      invalidate();
      os::System::instance()->setTranslateDeadKeys(true);
      break;
    }
    case kFocusLeaveMessage: {
      stopTimer();
      m_drawCaret = false;
      invalidateRect(m_caretRect);
      os::System::instance()->setTranslateDeadKeys(false);
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
      Caret leftCaret = caretFromPosition(mouseMessage->position());
      if (!leftCaret.isValid())
        return false;

      Caret rightCaret = leftCaret;
      leftCaret.leftWord();
      rightCaret.rightWord();

      if (leftCaret != rightCaret) {
        m_selection.set(leftCaret, rightCaret);
        m_caret = rightCaret;
        invalidate();
        captureMouse();
        return true;
      }
      break;
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

        const auto* mouseMsg = static_cast<MouseMessage*>(msg);
        if (mouseMsg->right()) {
          showEditPopupMenu(mouseMsg->position());
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
      const auto* mouseMsg = static_cast<MouseMessage*>(msg);
      auto* view = View::getView(this);
      gfx::Point scroll = view->viewScroll();

      if (mouseMsg->preciseWheel())
        scroll += mouseMsg->wheelDelta();
      else
        scroll += mouseMsg->wheelDelta() * font()->height();

      view->setViewScroll(scroll);
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

bool TextEdit::onKeyDown(const KeyMessage* keyMessage)
{
  const KeyScancode scancode = keyMessage->scancode();
  const bool byWord = keyMessage->ctrlPressed();
  const Caret prevCaret(m_caret);

  switch (scancode) {
    case kKeyLeft:  m_caret.left(byWord); break;
    case kKeyRight: m_caret.right(byWord); break;
    case kKeyHome:  m_caret.setPos(0); break;
    case kKeyEnd:   m_caret.set(m_lines.back().i, m_lines.back().glyphCount); break;
    case kKeyUp:    m_caret.up(); break;
    case kKeyDown:  m_caret.down(); break;
    case kKeyEnter: {
      deleteSelection();

      std::string newText = text();
      newText.insert(m_caret.absolutePos(), "\n");
      setText(newText);

      m_caret.set(m_caret.line() + 1, 0);
      return true;
    }
    case kKeyBackspace: [[fallthrough]];
    case kKeyDel:       {
      if (m_selection.isEmpty() || !m_selection.isValid()) {
        Caret startCaret = m_caret;
        Caret endCaret = startCaret;

        if (scancode == kKeyBackspace) {
          startCaret.left(byWord);
        }
        else {
          endCaret.right(byWord);
        }

        m_selection.set(startCaret, endCaret);
      }

      deleteSelection();
      return true;
    }
    default:
      if (keyMessage->unicodeChar() >= 32) {
        deleteSelection();
        if (keyMessage->isDeadKey()) {
          return true;
        }

        insertCharacter(keyMessage->unicodeChar());
        return true;
      }
      if (scancode >= kKeyFirstModifierScancode) {
        return true;
      }
#if defined __APPLE__
      if (keyMessage->onlyCmdPressed())
#else
      if (keyMessage->onlyCtrlPressed())
#endif
      {
        switch (scancode) {
          case kKeyX: {
            cut();
            return true;
          }
          case kKeyC: {
            copy();
            return true;
          }
          case kKeyV: {
            paste();
            return true;
          }
          case kKeyA: {
            selectAll();
            return true;
          }
        }
      }
      return false;
  }

  // Selection modification
  if (keyMessage->shiftPressed()) {
    if (!m_selection.isValid() || m_selection.isEmpty()) {
      m_lockedSelectionStart = prevCaret;
    }

    m_selection.set(m_lockedSelectionStart, m_caret);
  }
  else
    m_selection.clear();

  return true;
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
  g->fillRect(theme()->getColorById("textbox_face"), rect);

  const auto& scroll = view->viewScroll();
  gfx::PointF point(border().left(), border().top());
  point -= scroll;

  m_caretRect =
    gfx::Rect(border().left() - scroll.x, border().top() - scroll.y, 2, font()->height());

  os::Paint textPaint;
  textPaint.color(theme()->getColorById("text"));
  textPaint.style(os::Paint::Fill);

  os::Paint selectedTextPaint;
  selectedTextPaint.color(theme()->getColorById("selected_text"));
  selectedTextPaint.style(os::Paint::Fill);

  const gfx::Rect clipBounds = g->getClipBounds();

  for (const auto& line : m_lines) {
    const bool caretLine = (line.i == m_caret.line());

    // Skip drawing lines when they're out of scroll bounds or they're outside the clip bounds,
    // unless we're in the caret line, in which case we need to draw the text to avoid blank
    // characters.
    const bool skip =
      (point.y + line.height < scroll.y || point.y > scroll.y + rect.h) ||
      (!clipBounds.intersects(gfx::Rect(point.x, point.y, line.width, line.height)) && !caretLine);

    if (!skip) {
      g->drawTextBlob(line.blob, point, textPaint);

      // Drawing the selection rect and any selected text.
      // We're technically drawing over the old text, so ideally we want to clip that off as well?
      const gfx::RectF selectionRect = getSelectionRect(line, point);
      if (!selectionRect.isEmpty()) {
        g->fillRect(theme()->getColorById("selected"), selectionRect);

        const IntersectClip clip(g, selectionRect);
        if (clip)
          g->drawTextBlob(line.blob, point, selectedTextPaint);
      }
    }

    // If we're in the caret's line, run this blob to grab where we should position it.
    if (caretLine) {
      if (m_caret.isLastInLine()) {
        m_caretRect.x += line.width;
      }
      else if (m_caret.pos() > 0) {
        m_caretRect.x += line.getBounds(m_caret.pos()).x;
      }

      m_caretRect.y = point.y;
      m_caretRect.h = line.height; // Ensure the caret height corresponds with the tallest glyph
    }

    point.y += line.height;
  }

  if (m_drawCaret)
    g->drawRect(theme()->getColorById("text"), m_caretRect);

  m_caretRect.offset(gfx::Point(g->getInternalDeltaX(), g->getInternalDeltaY()));
}

void TextEdit::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(m_textSize);
}

void TextEdit::onScrollRegion(ScrollRegionEvent& ev)
{
  invalidateRegion(ev.region());
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
                                            m_lines[m_selection.start().line()].glyphCount);
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
  gfx::PointF offsetPosition(position.x - (bounds().x + border().left()),
                             position.y - (bounds().y + border().top()));

  offsetPosition += View::getView(this)->viewScroll();

  Caret caret(&m_lines);
  const int lineHeight = font()->height();

  // First check if the offset position is blank (below all the lines)
  if (offsetPosition.y > m_lines.size() * lineHeight) {
    // Get the last character in the last line.
    caret.setLine(m_lines.size() - 1);

    // Check the line width and if we're more than halfway past the line, we can set the caret to
    // the full line.
    caret.setPos(
      (offsetPosition.x > m_lines[caret.line()].width / 2) ? m_lines[caret.line()].glyphCount : 0);
    return caret;
  }

  for (const Line& line : m_lines) {
    const size_t lineStartY = line.i * lineHeight;
    const size_t lineEndY = (line.i + 1) * lineHeight;

    if (offsetPosition.y > lineEndY || offsetPosition.y < lineStartY)
      continue; // We're not in this line

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
    size_t advance = 0;
    bool found = false;

    line.blob->visitRuns([&](const text::TextBlob::RunInfo& run) {
      if (found) {
        return;
      }

      for (int i = 0; i < run.glyphCount; ++i) {
        gfx::RectF glyphBounds = run.getGlyphBounds(i).offset(gfx::PointF(0, lineStartY));

        if (glyphBounds.contains(offsetPosition)) {
          found = true;

          if (offsetPosition.x > glyphBounds.center().x && advance != line.glyphCount)
            ++advance; // If the mouse is to the right of the glyph, prefer the next position.

          return;
        }

        ++advance;
      }
    });

    if (found) {
      caret.setPos(advance);
    }
  }

  return caret;
}

void TextEdit::showEditPopupMenu(const gfx::Point& position)
{
  auto* translate = UISystem::instance()->translationDelegate();
  ASSERT(translate); // We provide UISystem as default translation delegate
  if (!translate)
    return;

  Menu menu;
  MenuItem cut(translate->cut());
  MenuItem copy(translate->copy());
  MenuItem paste(translate->paste());
  MenuItem selectAll(translate->selectAll());

  cut.processMnemonicFromText();
  copy.processMnemonicFromText();
  paste.processMnemonicFromText();
  selectAll.processMnemonicFromText();

  menu.addChild(&cut);
  menu.addChild(&copy);
  menu.addChild(&paste);
  menu.addChild(new MenuSeparator);
  menu.addChild(&selectAll);

  cut.setEnabled(!m_selection.isEmpty());
  copy.setEnabled(!m_selection.isEmpty());

  cut.Click.connect(&TextEdit::cut, this);
  copy.Click.connect(&TextEdit::copy, this);
  paste.Click.connect(&TextEdit::paste, this);
  selectAll.Click.connect(&TextEdit::selectAll, this);

  menu.showPopup(position, display());
}

void TextEdit::insertCharacter(base::codepoint_t character)
{
  const std::string unicodeStr = base::codepoint_to_utf8(character);

  if (m_lines.empty()) {
    // Fast path for the first char.
    setText(unicodeStr);
    m_caret.setPos(m_caret.pos() + 1);
    return;
  }

  auto& line = m_lines[m_caret.line()];
  line.insertText(m_caret.pos(), unicodeStr);
  line.buildBlob(this);

  std::string newText = text();
  newText.insert(m_caret.absolutePos(), unicodeStr);
  setTextQuiet(newText);
  Change();

  m_caret.setPos(m_caret.pos() + 1);
}

void TextEdit::deleteSelection()
{
  if (m_selection.isEmpty() || !m_selection.isValid())
    return;

  std::string newText = text();
  newText.erase(newText.begin() + m_selection.start().absolutePos(),
                newText.begin() + m_selection.end().absolutePos());

  if (m_selection.start().line() == m_selection.end().line()) {
    auto& line = m_lines[m_selection.start().line()];
    size_t end;
    if (m_selection.end().isLastInLine())
      end = line.utfSize.back().end;
    else
      end = line.utfSize[m_selection.end().pos()].begin;

    line.text.erase(line.text.begin() + line.utfSize[m_selection.start().pos()].begin,
                    line.text.begin() + end);
    line.buildBlob(this);

    // Only rebuilds the one line
    setTextQuiet(newText);
    Change();
  }
  else {
    setText(newText);
  }

  m_caret = m_selection.start();
  m_selection.clear();
}

void TextEdit::ensureCaretVisible()
{
  auto* view = View::getView(this);
  if (!view || !view->hasScrollBars() || !m_caret.isValid())
    return;

  const int scrollBarWidth = theme()->getScrollbarSize();

  if (view->viewportBounds().shrink(scrollBarWidth).intersects(m_caretRect))
    return; // We are visible and don't need to do anything.

  const int lineHeight = font()->height();
  gfx::Point scroll = view->viewScroll();
  const gfx::Size visibleBounds = view->viewportBounds().size();

  if (view->verticalBar()->isVisible()) {
    const int heightLimit = (visibleBounds.h + scroll.y - lineHeight) / 2;
    const size_t currentLine = (m_caret.line() * lineHeight) / 2;

    if (currentLine <= scroll.y)
      scroll.y = currentLine;
    else if (currentLine >= heightLimit) // TODO: I do not like this
      scroll.y = currentLine - ((visibleBounds.h - (lineHeight * 2)) / 2);
  }

  const auto& line = m_lines[m_caret.line()];
  if (view->horizontalBar()->isVisible() && line.blob && line.width > visibleBounds.w) {
    const int caretX = line.getBounds(0, m_caret.pos()).w;
    const int horizontalLimit = scroll.x + visibleBounds.w - view->horizontalBar()->getBarWidth();

    if (m_caret.pos() == 0)
      scroll.x = 0;
    else if (caretX > horizontalLimit)
      scroll.x = caretX - horizontalLimit;
    else if (scroll.x > caretX / 2) // TODO: Something's a bit bouncy here (in a bad way)
      scroll.x = caretX / 2;
  }

  view->setViewScroll(scroll);
}

void TextEdit::onSetText()
{
  std::vector<std::string_view> newLines;
  newLines.reserve(m_lines.size()); // Assume lines will be around the same size as before, if any

  // Recalculate all the lines based on the widget's text
  m_lines.clear();

  base::split_string(text(), newLines, "\n");
  m_lines.reserve(newLines.size());

  int longestWidth = 0;
  int totalHeight = 0;

  for (const auto& lineString : newLines) {
    Line newLine;
    newLine.text = lineString;
    newLine.buildBlob(this);

    longestWidth = std::max(newLine.width, longestWidth);
    totalHeight += newLine.height;

    newLine.i = m_lines.size();
    m_lines.push_back(newLine);
  }

  m_textSize.w = longestWidth;
  m_textSize.h = totalHeight;

  if (auto* view = View::getView(this))
    view->updateView();

  Change();
  Widget::onSetText();
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

} // namespace ui
