// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/resize_event.h"
#include "ui/widget.h"

namespace ui {

using namespace gfx;

ResizeEvent::ResizeEvent(Widget* source, const gfx::Rect& bounds) : Event(source), m_bounds(bounds)
{
}

} // namespace ui
