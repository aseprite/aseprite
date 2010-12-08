// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_ENTRY_H_INCLUDED
#define GUI_ENTRY_H_INCLUDED

#include "base/signal.h"
#include "gui/widget.h"

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

  // for themes
  void getEntryThemeInfo(int* scroll, int* caret, int* state,
			 int* selbeg, int* selend);

  // Signals
  Signal0<void> EntryChange;

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

  // New Events
  void onEntryChange();

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

  int getCaretFromMouse(JMessage msg);
  void executeCmd(EntryCmd::Type cmd, int ascii, bool shift_pressed);
  void forwardWord();
  void backwardWord();

  size_t m_maxsize;
  int m_caret;
  int m_scroll;
  int m_select;
  int m_timer_id;
  bool m_hidden : 1;
  bool m_state : 1;		// show or not the text caret
  bool m_readonly : 1;
  bool m_password : 1;
  bool m_recent_focused : 1;
};

#endif
