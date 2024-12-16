// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_RESIZE_EVENT_H_INCLUDED
#define UI_RESIZE_EVENT_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "ui/event.h"

namespace ui {

class Widget;

class ResizeEvent : public Event {
public:
  ResizeEvent(Widget* source, const gfx::Rect& bounds);

  const gfx::Rect& bounds() { return m_bounds; }

private:
  gfx::Rect m_bounds;
};

} // namespace ui

#endif
