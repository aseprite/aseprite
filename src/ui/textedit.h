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

  bool onKeyDown(const KeyMessage* keyMessage);
  bool onMouseMove(const MouseMessage* mouseMessage);

private:
  struct Utf8RangeBuilder : public text::TextBlob::RunHandler {
    explicit Utf8RangeBuilder(size_t minSize) { ranges.reserve(minSize); }

    void commitRunBuffer(TextBlob::RunInfo& info) override
    {
      ASSERT(info.clusters == nullptr || *info.clusters == 0);
      for (size_t i = 0; i < info.glyphCount; ++i) {
        ranges.push_back(info.getGlyphUtf8Range(i));
      }
    }

    std::vector<TextBlob::Utf8Range> ranges;
  };

  struct Line {
    std::string text;
    std::vector<TextBlob::Utf8Range> utfSize;
    size_t glyphCount = 0;
    text::TextBlobRef blob;

    int width = 0;
    int height = 0;

    // Line index for more convenient loops
    size_t i = 0;

    void buildBlob(const Widget* forWidget)
    {
      utfSize.clear();

      if (text.empty()) {
        blob = nullptr;
        width = 0;
        glyphCount = 0;
        height = forWidget->font()->metrics(nullptr);
        return;
      }

      Utf8RangeBuilder rangeBuilder(text.size());
      blob = text::TextBlob::MakeWithShaper(forWidget->theme()->fontMgr(),
                                            forWidget->font(),
                                            text,
                                            &rangeBuilder);

      utfSize = std::move(rangeBuilder.ranges);
      glyphCount = utfSize.size();

      width = blob->bounds().w;
      height = std::max(blob->bounds().h, forWidget->font()->metrics(nullptr));
    }

    // Insert text into this line based on a caret position, taking into account utf8 size.
    void insertText(size_t pos, const std::string& str)
    {
      if (pos == 0)
        text.insert(0, str);
      else if (pos == glyphCount)
        text.append(str);
      else
        text.insert(utfSize[pos - 1].end, str);
    }

    gfx::Rect getBounds(size_t glyph) const
    {
      size_t advance = 0;
      gfx::Rect result;
      blob->visitRuns([&](const text::TextBlob::RunInfo& run) {
        for (size_t i = 0; i < run.glyphCount; ++i) {
          if (advance == glyph) {
            result = run.getGlyphBounds(i);
            return;
          }
          ++advance;
        }
      });
      return result;
    }

    // Get the screen size between the start and end glyph positions.
    gfx::Rect getBounds(size_t startGlyph, size_t endGlyph) const
    {
      if (startGlyph == endGlyph)
        return getBounds(startGlyph);

      ASSERT(endGlyph > startGlyph);

      size_t advance = 0; // The amount of glyphs we've advanced through.
      gfx::Rect resultBounds;

      blob->visitRuns([&](text::TextBlob::RunInfo& run) {
        if (advance >= endGlyph)
          return;

        if (startGlyph > (advance + run.glyphCount)) {
          advance += run.glyphCount;
          return; // Skip this run
        }

        size_t j = 0;
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

    void setPos(size_t pos)
    {
      ASSERT(pos >= 0 && pos <= lineObj().glyphCount);
      m_pos = pos;
    }

    void setLine(size_t line) { m_line = line; }

    void set(size_t line, size_t pos)
    {
      m_line = line;
      m_pos = pos;
    }

    bool left(bool byWord = false)
    {
      if (byWord)
        return leftWord();

      m_pos -= 1;

      if (((int64_t)(m_pos)-1) < 0) {
        if (m_line == 0) {
          m_pos = 0;
          return false;
        }

        m_line -= 1;
        m_pos = lineObj().glyphCount;
      }

      return true;
    }

    // Moves the position to the next word on the left, doesn't wrap around lines.
    bool leftWord()
    {
      if (m_pos == 0)
        return false;

      auto startPos = m_pos;
      while (isWordPart(m_pos)) {
        if (!left())
          return m_pos != startPos;
      }
      return true;
    }

    bool right(bool byWord = false)
    {
      if (byWord)
        return rightWord();

      m_pos += 1;

      if (m_pos > lineObj().glyphCount) {
        if (m_line == m_lines->size() - 1) {
          m_pos -= 1; // Undo movement, we've reached the end of the text.
          return false;
        }

        m_line += 1;
        m_pos = 0;
      }

      return true;
    }

    // Moves the position to the next word on the right, doesn't wrap around lines.
    bool rightWord()
    {
      if (m_pos == lineObj().glyphCount)
        return false;

      auto startPos = m_pos;
      while (isWordPart(m_pos)) {
        if (!right())
          return m_pos != startPos;
      }
      return true;
    }

    void up()
    {
      m_line = std::clamp(m_line - 1, size_t(0), m_lines->size() - 1);
      m_pos = std::clamp(m_pos, size_t(0), lineObj().glyphCount);
    }

    void down()
    {
      m_line = std::clamp(m_line + 1, size_t(0), m_lines->size() - 1);
      m_pos = std::clamp(m_pos, size_t(0), lineObj().glyphCount);
    }

    bool isLastInLine() const { return m_pos == lineObj().glyphCount; }

    bool isLastLine() const { return m_line == m_lines->size() - 1; }

    // Returns the absolute position of the caret, aka the position in the main string that has all
    // the newlines.
    size_t absolutePos() const
    {
      if (m_pos == 0 && m_line == 0)
        return 0;

      size_t apos = 0;
      for (const auto& l : *m_lines) {
        const bool hasNextLine = l.i < (m_lines->size() - 1);

        if (l.i == m_line) {
          if (l.text.empty() || m_pos == 0)
            return apos;

          if (m_pos >= l.utfSize.size())
            apos += l.utfSize.back().end;
          else if (m_pos > l.utfSize.size())
            apos += l.utfSize.back().end + (hasNextLine ? 1 : 0);
          else
            apos += l.utfSize[m_pos].begin;
          return apos;
        }

        if (!l.text.empty() && !l.utfSize.empty())
          apos += l.utfSize.back().end;

        if (hasNextLine)
          apos += 1; // Newline glyph.
      }
      return apos;
    }

    bool isWordPart(size_t pos) const
    {
      if (!lineObj().glyphCount || lineObj().utfSize.size() <= pos)
        return false;

      const auto& utfPos = lineObj().utfSize[pos];
      const std::string_view word = text().substr(utfPos.begin, utfPos.end - utfPos.begin);
      return (!word.empty() && std::isspace(word[0]) == 0 && std::ispunct(word[0]) == 0);
    }

    void advanceBy(size_t characters)
    {
      size_t remaining = characters;
      size_t activeLine = m_line;
      for (size_t i = m_line; i < m_lines->size(); ++i) {
        const auto& line = (*m_lines)[i];
        for (size_t j = m_pos; j < line.glyphCount; ++j) {
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

    bool isValid() const
    {
      if (m_lines == nullptr)
        return false;

      if (m_line >= m_lines->size())
        return false;

      if (m_pos > lineObj().glyphCount)
        return false;

      return true;
    }

    void clear()
    {
      m_lines = nullptr;
      m_line = 0;
      m_pos = 0;
    }

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

    void set(const Caret& startCaret, const Caret& endCaret)
    {
      if (startCaret > endCaret) {
        m_start = endCaret;
        m_end = startCaret;
      }
      else {
        m_start = startCaret;
        m_end = endCaret;
      }
    }

    const Caret& start() const { return m_start; }

    const Caret& end() const { return m_end; }

    bool isValid() const { return m_start.isValid() && m_end.isValid(); }

    void clear()
    {
      m_start.clear();
      m_end.clear();
    }

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
};

} // namespace ui

#endif
