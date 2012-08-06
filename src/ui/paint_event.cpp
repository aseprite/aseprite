// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/paint_event.h"
#include "ui/widget.h"

namespace ui {

PaintEvent::PaintEvent(Widget* source, Graphics* graphics)
  : Event(source)
  , m_graphics(graphics)
  , m_painted(false)
{
}

PaintEvent::~PaintEvent()
{
}

Graphics* PaintEvent::getGraphics()
{
  // If someone requested the graphics pointer, it means that this
  // "someone" has painted the widget.
  m_painted = true;
  return m_graphics;
}

bool PaintEvent::isPainted() const
{
  return m_painted;
}

} // namespace ui
