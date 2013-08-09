// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_RESIZE_EVENT_H_INCLUDED
#define UI_RESIZE_EVENT_H_INCLUDED

#include "gfx/rect.h"
#include "ui/event.h"

namespace ui {

  class Widget;

  class ResizeEvent : public Event
  {
  public:
    ResizeEvent(Widget* source, const gfx::Rect& bounds);

    const gfx::Rect& getBounds() { return m_bounds; }

  private:
    gfx::Rect m_bounds;
  };

} // namespace ui

#endif
