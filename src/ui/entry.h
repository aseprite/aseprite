// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_ENTRY_H_INCLUDED
#define UI_ENTRY_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/timer.h"
#include "ui/widget.h"

namespace ui {

  class MouseMessage;

  class Entry : public Widget
  {
  public:
    Entry(size_t maxsize, const char *format, ...);
    ~Entry();

    bool isPassword() const;
    bool isReadOnly() const;
    void setReadOnly(bool state);
    void setPassword(bool state);

    void showCaret();
    void hideCaret();

    void setCaretPos(int pos);
    void selectText(int from, int to);
    void deselectText();

    void setSuffix(const std::string& suffix);
    const std::string& getSuffix() { return m_suffix; }

    // for themes
    void getEntryThemeInfo(int* scroll, int* caret, int* state,
                           int* selbeg, int* selend);

    // Signals
    Signal0<void> EntryChange;

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onSetText() OVERRIDE;

    // New Events
    virtual void onEntryChange();

  private:
    struct EntryCmd {
      enum Type {
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
        Cut,
        Copy,
        Paste,
      };
    };

    int getCaretFromMouse(MouseMessage* mousemsg);
    void executeCmd(EntryCmd::Type cmd, int ascii, bool shift_pressed);
    void forwardWord();
    void backwardWord();
    int getAvailableTextLength();

    size_t m_maxsize;
    int m_caret;
    int m_scroll;
    int m_select;
    Timer m_timer;
    bool m_hidden : 1;
    bool m_state : 1;             // show or not the text caret
    bool m_readonly : 1;
    bool m_password : 1;
    bool m_recent_focused : 1;
    std::string m_suffix;
  };

} // namespace ui

#endif
