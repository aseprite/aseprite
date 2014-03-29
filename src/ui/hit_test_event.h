// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_HIT_TEST_EVENT_H_INCLUDED
#define UI_HIT_TEST_EVENT_H_INCLUDED
#pragma once

#include "ui/event.h"

namespace ui {

  enum HitTest
    {
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

  class HitTestEvent : public Event
  {
  public:
    HitTestEvent(Component* source, const gfx::Point& point, HitTest hit)
      : Event(source)
      , m_point(point)
      , m_hit(hit) { }

    gfx::Point getPoint() const { return m_point; }

    HitTest getHit() const { return m_hit; }
    void setHit(HitTest hit) { m_hit = hit; }

  private:
    gfx::Point m_point;
    HitTest m_hit;
  };

} // namespace ui

#endif  // UI_HIT_TEST_EVENT_H_INCLUDED
