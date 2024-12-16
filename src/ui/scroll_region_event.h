// Aseprite UI Library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCROLL_REGION_EVENT_H_INCLUDED
#define UI_SCROLL_REGION_EVENT_H_INCLUDED
#pragma once

#include "gfx/region.h"
#include "ui/event.h"

namespace ui {

class ScrollRegionEvent : public Event {
public:
  ScrollRegionEvent(Component* source, gfx::Region& region) : Event(source), m_region(region) {}

  gfx::Region& region() { return m_region; }

private:
  gfx::Region& m_region;
};

} // namespace ui

#endif
