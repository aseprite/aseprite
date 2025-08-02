// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef UI_TEXT_EDIT_H_INCLUDED
#define UI_TEXT_EDIT_H_INCLUDED
#pragma once

#include "text/font_mgr.h"
#include "text/text_blob.h"
#include "ui/box.h"
#include "ui/theme.h"
#include "ui/view.h"

namespace ui {
using namespace text;

class TextEdit : public Widget,
                 public ViewableWidget {
public:
  TextEdit();

  void cut();
  void copy();
  void paste();
  void selectAll();

  obs::signal<void()> Change;

protected:
  bool onProcessMessage(Message* msg) override;
  void onPaint(PaintEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onScrollRegion(ScrollRegionEvent& ev) override;
  void onSetText() override;
  void onSetFont() override;

  bool onKeyDown(const KeyMessage* keyMessage);
  bool onMouseMove(const MouseMessage* mouseMessage);

private:
  struct Line {
    std::string text;
    std::vector<TextBlob::Utf8Range> utfSize;
    size_t glyphCount = 0;
    text::TextBlobRef blob;

    int width = 0;
    int height = 0;

    // Line index for more convenient loops
    size_t i = 0;

    void buildBlob(const Widget* forWidget);

    // Insert text into this line based on a caret position, taking into account utf8 size.
    void insertText(size_t pos, const std::string& str);

    gfx::Rect getBounds(size_t glyph) const;

    // Get the screen size between the start and end glyph positions.
    gfx::Rect getBounds(size_t startGlyph, size_t endGlyph) const;
  };

  struct Caret {
    explicit Caret(std::vector<Line>* lines = nullptr) : m_lines(lines) {}
    explicit Caret(std::vector<Line>* lines, size_t line, size_t pos)
      : m_line(line)
      , m_pos(pos)
      , m_lines(lines)
    {
    }
    Caret(const Caret& caret) : m_line(caret.m_line), m_pos(caret.m_pos), m_lines(caret.m_lines) {}

    size_t line() const { return m_line; }
    size_t pos() const { return m_pos; }

    void setPos(size_t pos);
    void setLine(size_t line) { m_line = line; }
    void set(size_t line, size_t pos);

    bool left(bool byWord = false);
    // Moves the position to the next word on the left, doesn't wrap around lines.
    bool leftWord();
    bool right(bool byWord = false);
    // Moves the position to the next word on the right, doesn't wrap around lines.
    bool rightWord();
    void up();
    void down();
    bool isLastInLine() const { return m_pos == lineObj().glyphCount; }
    bool isLastLine() const { return m_line == m_lines->size() - 1; }

    // Returns the absolute position of the caret, aka the position in the main string that has all
    // the newlines.
    size_t absolutePos() const;
    bool isWordPart(size_t pos) const;
    void advanceBy(size_t characters);
    bool isValid() const;
    void clear();

    bool operator==(const Caret& other) const
    {
      return m_line == other.m_line && m_pos == other.m_pos;
    }

    bool operator!=(const Caret& other) const
    {
      return m_line != other.m_line || m_pos != other.m_pos;
    }

    bool operator>(const Caret& other) const
    {
      return (m_line == other.m_line) ? m_pos > other.m_pos :
                                        (m_line + m_pos) > (other.m_line + m_pos);
    }

  private:
    size_t m_line = 0;
    size_t m_pos = 0;
    std::string_view text() const { return (*m_lines)[m_line].text; }
    Line& lineObj() const { return (*m_lines)[m_line]; }
    std::vector<Line>* m_lines;
  };

  struct Selection {
    Selection() = default;
    Selection(const Caret& startCaret, const Caret& endCaret) { set(startCaret, endCaret); }

    bool isEmpty() const
    {
      return (m_start.line() == m_end.line() && m_start.pos() == m_end.pos());
    }

    void setStart(const Caret& caret) { m_start = caret; }
    void setEnd(const Caret& caret) { m_end = caret; }
    void set(const Caret& startCaret, const Caret& endCaret);

    const Caret& start() const { return m_start; }
    const Caret& end() const { return m_end; }
    bool isValid() const { return m_start.isValid() && m_end.isValid(); }

    void clear();

  private:
    Caret m_start;
    Caret m_end;
  };

  // Get the selection rect for the given line, if any
  gfx::RectF getSelectionRect(const Line& line, const gfx::PointF& offset) const;
  Caret caretFromPosition(const gfx::Point& position);
  void showEditPopupMenu(const gfx::Point& position);
  void insertCharacter(base::codepoint_t character);
  void deleteSelection();
  void ensureCaretVisible();

  void startTimer();
  void stopTimer();

  Selection m_selection;
  Caret m_caret;
  Caret m_lockedSelectionStart;

  std::vector<Line> m_lines;

  // Whether or not we're currently drawing the caret, driven by a timer.
  bool m_drawCaret = false;

  // The last position the caret was drawn, to invalidate that region when repainting.
  gfx::Rect m_caretRect;

  // The total size of the complete text, calculated as the longest single line width and the sum of
  // the total line heights
  gfx::Size m_textSize;

  // Color cache
  gfx::Color m_colorBG;
  gfx::Color m_colorSelected;
  os::Paint m_textPaint;
  os::Paint m_selectedTextPaint;
};

} // namespace ui

#endif
