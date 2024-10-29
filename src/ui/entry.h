// Aseprite UI Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_ENTRY_H_INCLUDED
#define UI_ENTRY_H_INCLUDED
#pragma once

#include "base/codepoint.h"
#include "obs/signal.h"
#include "ui/widget.h"

#include <memory>

namespace ui {

  class MouseMessage;

  class Entry : public Widget {
  public:
    struct Range {
      int from = -1;
      int to = -1;
      Range() = default;
      Range(int from, int to) : from(from), to(to) { }
      bool isEmpty() const { return from < 0; }
      int size() const { return to - from; }
      void reset() { from = to = -1; }
    };

    Entry(const int maxsize, const char *format, ...);
    virtual ~Entry();

    void setMaxTextLength(const int maxsize);

    bool isReadOnly() const;
    void setReadOnly(bool state);

    bool isMultiline() const;
    void setMultiline(bool state);

    void showCaret();
    void hideCaret();

    int caretPos() const { return m_caret; }
    int lastCaretPos() const;

    void setCaretPos(int pos);
    void setCaretToEnd();
    bool isCaretVisible() const { return !m_hidden && m_state; }

    void selectText(int from, int to);
    void selectAllText();
    void deselectText();
    std::string selectedText() const;
    Range selectedRange() const;

    // Set to true if you want to persists the selection when the
    // keyboard focus is lost/re-enters.
    void setPersistSelection(bool state) { m_persistSelection = state; }

    void setSuffix(const std::string& suffix);
    std::string getSuffix();

    void setTranslateDeadKeys(bool state);

    // for themes
    void getEntryThemeInfo(int* scroll,
                           int* line_scroll,
                           int* caret,
                           int* state,
                           Range* range) const;
    gfx::Rect getEntryTextBounds() const;

    gfx::PointF scale() const { return m_scale; }
    void setScale(const gfx::PointF& scale) { m_scale = scale; }

    static gfx::Size sizeHintWithText(Entry* entry,
                                      const std::string& text);

    // Signals
    obs::signal<void()> Change;

  protected:
    gfx::Rect getCharBoxBounds(int i);

    // Events
    bool onProcessMessage(Message* msg) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
    void onSetFont() override;
    void onSetText() override;

    // New Events
    virtual void onChange();
    virtual gfx::Rect onGetEntryTextBounds() const;

  private:
    enum class EntryCmd {
      NoOp,
      InsertChar,
      ForwardChar,
      ForwardWord,
      BackwardChar,
      BackwardWord,
      BeginningOfLine,
      EndOfLine,
      InsertNewLine,
      LineDown,
      LineUp,
      DeleteForward,
      DeleteBackward,
      DeleteBackwardWord,
      DeleteForwardToEndOfLine,
      Cut,
      Copy,
      Paste,
      SelectAll,
    };

    struct EntryLine {
      int start = -1;
      int end = -1;
    };

    int getCaretFromMouse(MouseMessage* mousemsg);
    void executeCmd(EntryCmd cmd, base::codepoint_t unicodeChar, bool shift_pressed);
    void forwardWord();
    void backwardWord();
    Range wordRange(int pos);
    bool isPosInSelection(int pos) const;
    void showEditPopupMenu(const gfx::Point& pt);
    void recalcCharBoxes(const std::string& text);
    bool shouldStartTimer(const bool hasFocus);
    void deleteRange(const Range& range, std::string& text);
    void startTimer();
    void stopTimer();
    int caretToLine(const int caret_position);

    class CalcBoxesTextDelegate;

    struct CharBox {
      int codepoint = 0;
      int from = 0;
      int to = 0;
      float x = 0.0f;
      float y = 0.0f;
      float width = 0.0f;
    };

    using CharBoxes = std::vector<CharBox>;
    using EntryLines = std::vector<EntryLine>;

    CharBoxes m_boxes;
    EntryLines m_lines;
    int m_maxsize;
    int m_caret;
    int m_select;
    int m_horizontalScroll;
    int m_verticalLineScroll;

    bool m_hidden : 1;
    bool m_state : 1;             // show or not the text caret
    bool m_readonly : 1;
    bool m_recentlyFocused : 1;
    bool m_lockSelection : 1;
    bool m_persistSelection : 1;
    bool m_translateDeadKeys : 1;
    bool m_multiline : 1;
    Range m_selectingWords;
    std::unique_ptr<std::string> m_suffix;

    // Scale (1.0 by default) applied to each axis. Can be used in
    // case you are going to display/paint the text scaled and want to
    // convert the mouse position correctly.
    gfx::PointF m_scale;
  };

} // namespace ui

#endif
