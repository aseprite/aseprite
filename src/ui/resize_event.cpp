// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/resize_event.h"
#include "ui/widget.h"

using namespace gfx;

namespace ui {

ResizeEvent::ResizeEvent(Widget* source, const gfx::Rect& bounds)
  : Event(source)
  , m_bounds(bounds)
{
}

} // namespace ui
