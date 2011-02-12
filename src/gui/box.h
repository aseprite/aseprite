// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_BOX_H_INCLUDED
#define GUI_BOX_H_INCLUDED

#include "gui/widget.h"

class Box : public Widget
{
public:
  Box(int align);

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
  void onPaint(PaintEvent& ev);

private:
  void box_set_position(JRect rect);
};

#endif
