// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/size_hint_event.h"
#include "ui/widget.h"

namespace ui {

using namespace gfx;

/**
   Event generated to calculate the preferred size of a widget.

   @param source
     The widget that want to know its preferred size.

   @param fitIn
     This could be Size(0, 0) that means calculate the preferred size
     without restrictions. If its width or height is greater than 0,
     you could try to fit your widget to that width or height.
*/
SizeHintEvent::SizeHintEvent(Widget* source, const Size& fitIn)
  : Event(source)
  , m_fitIn(fitIn)
  , m_sizeHint(0, 0)
{
}

/**
   Destroys the SizeHintEvent.
*/
SizeHintEvent::~SizeHintEvent()
{
}

Size SizeHintEvent::fitInSize() const
{
  return m_fitIn;
}

int SizeHintEvent::fitInWidth() const
{
  return m_fitIn.w;
}

int SizeHintEvent::fitInHeight() const
{
  return m_fitIn.h;
}

Size SizeHintEvent::sizeHint() const
{
  return m_sizeHint;
}

void SizeHintEvent::setSizeHint(const Size& sz)
{
  m_sizeHint = sz;
}

/**
   Sets the preferred size for the widget.
*/
void SizeHintEvent::setSizeHint(int w, int h)
{
  m_sizeHint.w = w;
  m_sizeHint.h = h;
}

} // namespace ui
