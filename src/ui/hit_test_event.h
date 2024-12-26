// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_HIT_TEST_EVENT_H_INCLUDED
#define UI_HIT_TEST_EVENT_H_INCLUDED
#pragma once

#include "ui/event.h"

namespace ui {

enum HitTest {
  HitTestNowhere,
  HitTestCaption,
  HitTestClient,
  HitTestBorderNW,
  HitTestBorderN,
  HitTestBorderNE,
  HitTestBorderE,
  HitTestBorderSE,
  HitTestBorderS,
  HitTestBorderSW,
  HitTestBorderW,
};

class HitTestEvent : public Event {
public:
  HitTestEvent(Component* source, const gfx::Point& point, HitTest hit)
    : Event(source)
    , m_point(point)
    , m_hit(hit)
  {
  }

  gfx::Point point() const { return m_point; }

  HitTest hit() const { return m_hit; }
  void setHit(HitTest hit) { m_hit = hit; }

private:
  gfx::Point m_point;
  HitTest m_hit;
};

} // namespace ui

#endif // UI_HIT_TEST_EVENT_H_INCLUDED
