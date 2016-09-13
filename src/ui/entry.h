// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
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
    Entry(std::size_t maxsize, const char *format, ...);
    ~Entry();

    bool isPassword() const;
    bool isReadOnly() const;
    void setReadOnly(bool state);
    void setPassword(bool state);

    void showCaret();
    void hideCaret();

    void setCaretPos(int pos);
    void selectText(int from, int to);
    void selectAllText();
    void deselectText();

    void setSuffix(const std::string& suffix);
    const std::string& getSuffix() { return m_suffix; }

    // for themes
    void getEntryThemeInfo(int* scroll, int* caret, int* state,
                           int* selbeg, int* selend);
    gfx::Rect getEntryTextBounds() const;

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
    int getAvailableTextLength();
    bool isPosInSelection(int pos);
    void showEditPopupMenu(const gfx::Point& pt);

    Timer m_timer;
    std::size_t m_maxsize;
    int m_caret;
    int m_scroll;
    int m_select;
    bool m_hidden;
    bool m_state;             // show or not the text caret
    bool m_readonly;
    bool m_password;
    bool m_recent_focused;
    bool m_lock_selection;
    std::string m_suffix;
  };

} // namespace ui

#endif
