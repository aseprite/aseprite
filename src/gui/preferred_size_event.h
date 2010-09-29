// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_PREFERRED_SIZE_EVENT_H_INCLUDED
#define GUI_PREFERRED_SIZE_EVENT_H_INCLUDED

#include "gui/event.h"
#include "gfx/size.h"

class Widget;

class PreferredSizeEvent : public Event
{
  gfx::Size m_fitIn;
  gfx::Size m_preferredSize;

public:

  PreferredSizeEvent(Widget* source, const gfx::Size& fitIn);
  virtual ~PreferredSizeEvent();

  gfx::Size fitInSize() const;
  int fitInWidth() const;
  int fitInHeight() const;

  gfx::Size getPreferredSize() const;
  void setPreferredSize(const gfx::Size& preferredSize);
  void setPreferredSize(int w, int h);

};

#endif
