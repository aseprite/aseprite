// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/preferred_size_event.h"
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
PreferredSizeEvent::PreferredSizeEvent(Widget* source, const Size& fitIn)
  : Event(source)
  , m_fitIn(fitIn)
  , m_preferredSize(0, 0)
{
}

/**
   Destroys the PreferredSizeEvent.
*/
PreferredSizeEvent::~PreferredSizeEvent()
{
}

Size PreferredSizeEvent::fitInSize() const
{
  return m_fitIn;
}

int PreferredSizeEvent::fitInWidth() const
{
  return m_fitIn.w;
}

int PreferredSizeEvent::fitInHeight() const
{
  return m_fitIn.h;
}

Size PreferredSizeEvent::getPreferredSize() const
{
  return m_preferredSize;
}

void PreferredSizeEvent::setPreferredSize(const Size& preferredSize)
{
  m_preferredSize = preferredSize;
}

/**
   Sets the preferred size for the widget.
*/
void PreferredSizeEvent::setPreferredSize(int w, int h)
{
  m_preferredSize.w = w;
  m_preferredSize.h = h;
}

} // namespace ui
