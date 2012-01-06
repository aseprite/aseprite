// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_PAINT_EVENT_H_INCLUDED
#define GUI_PAINT_EVENT_H_INCLUDED

#include "gui/event.h"

class Graphics;
class Widget;

class PaintEvent : public Event
{
public:
  PaintEvent(Widget* source, Graphics* graphics);
  virtual ~PaintEvent();

  Graphics* getGraphics();

  bool isPainted() const;

private:
  Graphics* m_graphics;
  bool m_painted;
};

#endif
