// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_TEXTBOX_H_INCLUDED
#define GUI_TEXTBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

class TextBox : public Widget
{
public:
  TextBox(const char* text, int align);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  void onSetText() OVERRIDE;
};

#endif
