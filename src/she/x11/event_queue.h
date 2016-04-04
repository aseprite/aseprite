// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_EVENT_QUEUE_INCLUDED
#define SHE_X11_EVENT_QUEUE_INCLUDED
#pragma once

#include "she/event.h"
#include "she/event_queue.h"
#include "she/x11/x11.h"

#include <queue>

#pragma push_macro("None")
#undef None // Undefine the X11 None macro

namespace she {

class X11EventQueue : public EventQueue {
public:
  void getEvent(Event& ev, bool canWait) override {
    XEvent event;
    XNextEvent(X11::instance()->display(), &event);

    if (m_events.empty()) {
      ev.setType(Event::None);
    }
    else {
      ev = m_events.front();
      m_events.pop();
    }
  }

  void queueEvent(const Event& ev) override {
    m_events.push(ev);
  }

private:
  std::queue<Event> m_events;
};

typedef X11EventQueue EventQueueImpl;

} // namespace she

#pragma pop_macro("None")

#endif
