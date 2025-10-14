// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TEXTCMD_H_INCLUDED
#define UI_TEXTCMD_H_INCLUDED
#pragma once

#include "base/codepoint.h"
#include "gfx/point.h"

namespace ui {

class Display;
class KeyMessage;

class TextCmdProcessor {
public:
  enum Cmd {
    NoOp,
    InsertChar,
    PrevChar,
    PrevWord,
    PrevLine,
    NextChar,
    NextWord,
    NextLine,
    BegOfLine,
    EndOfLine,
    BegOfFile,
    EndOfFile,
    DeletePrevChar,
    DeleteNextChar,
    DeletePrevWord,
    DeleteNextWord,
    DeleteToEndOfLine,
    Cut,
    Copy,
    Paste,
    SelectAll,
  };

protected:
  Cmd cmdFromKeyMessage(const KeyMessage* msg);
  void executeCmd(Cmd cmd, base::codepoint_t unicodeChar = 0, bool expandSelection = false);
  void showEditPopupMenu(Display* display, const gfx::Point& pt);

  virtual bool onHasValidSelection() = 0;
  virtual bool onCanModify() = 0;
  virtual void onExecuteCmd(Cmd cmd, base::codepoint_t unicodeChar, bool expandSelection) = 0;
};

} // namespace ui

#endif
