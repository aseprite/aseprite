// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_HIT_TEST_EVENT_H_INCLUDED
#define GUI_HIT_TEST_EVENT_H_INCLUDED

#include "gui/event.h"

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

#endif  // GUI_HIT_TEST_EVENT_H_INCLUDED
