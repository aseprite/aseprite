// SHE library
// Copyright (C) 2016-2017  David Capello
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

namespace she {

class X11EventQueue : public EventQueue {
public:
  void getEvent(Event& ev, bool canWait) override;
  void queueEvent(const Event& ev) override {
    m_events.push(ev);
  }

private:
  void processX11Event(XEvent& event);

  std::queue<Event> m_events;
};

typedef X11EventQueue EventQueueImpl;

} // namespace she

#endif
