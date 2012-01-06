// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_VIEWPORT_H_INCLUDED
#define GUI_VIEWPORT_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

class Viewport : public Widget
{
public:
  Viewport();

  gfx::Size calculateNeededSize();

protected:
  // Events
  bool onProcessMessage(Message* msg) OVERRIDE;

private:
  void set_position(JRect rect);
};

#endif
