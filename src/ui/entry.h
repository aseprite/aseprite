// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_ENTRY_H_INCLUDED
#define UI_ENTRY_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/timer.h"
#include "ui/widget.h"

namespace ui {

  class MouseMessage;

  class Entry : public Widget {
  public:
    struct Range {
      int from = -1, to = -1;
      bool isEmpty() const { return from < 0; }
      int size() const { return to - from; }
    };

    Entry(const int maxsize, const char *format, ...);
    ~Entry();

    void setMaxTextLength(const int maxsize);

    bool isReadOnly() const;
    void setReadOnly(bool state);

    void showCaret();
    void hideCaret();

    int caretPos() const { return m_caret; }
    int lastCaretPos() const;

    void setCaretPos(int pos);
    void setCaretToEnd();

    void selectText(int from, int to);
    void selectAllText();
    void deselectText();
    std::string selectedText() const;
    Range selectedRange() const;

    void setSuffix(const std::string& suffix);
    const std::string& getSuffix() { return m_suffix; }

    void setTranslateDeadKeys(bool state);

    // for themes
    void getEntryThemeInfo(int* scroll, int* caret, int* state, Range* range) const;
    gfx::Rect getEntryTextBounds() const;

    static gfx::Size sizeHintWithText(Entry* entry,
                                      const std::string& text);

    // Signals
    obs::signal<void()> Change;

  protected:
    // Events
    bool onProcessMessage(Message* msg) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
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
      DeleteForward,
      DeleteBackward,
      DeleteBackwardWord,
      DeleteForwardToEndOfLine,
      Cut,
      Copy,
      Paste,
      SelectAll,
    };

    int getCaretFromMouse(MouseMessage* mousemsg);
    void executeCmd(EntryCmd cmd, int ascii, bool shift_pressed);
    void forwardWord();
    void backwardWord();
    bool isPosInSelection(int pos);
    void showEditPopupMenu(const gfx::Point& pt);
    void recalcCharBoxes(const std::string& text);
    bool shouldStartTimer(const bool hasFocus);
    void deleteRange(const Range& range, std::string& text);

    class CalcBoxesTextDelegate;

    struct CharBox {
      int codepoint;
      int from, to;
      int width;
      CharBox() { codepoint = from = to = width = 0; }
    };

    typedef std::vector<CharBox> CharBoxes;

    CharBoxes m_boxes;
    Timer m_timer;
    int m_maxsize;
    int m_caret;
    int m_scroll;
    int m_select;
    bool m_hidden;
    bool m_state;             // show or not the text caret
    bool m_readonly;
    bool m_recent_focused;
    bool m_lock_selection;
    bool m_translate_dead_keys;
    std::string m_suffix;
  };

} // namespace ui

#endif
