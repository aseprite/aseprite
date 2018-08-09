// LAF OS Library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_EVENT_QUEUE_INCLUDED
#define OS_X11_EVENT_QUEUE_INCLUDED
#pragma once

#include "os/event.h"
#include "os/event_queue.h"
#include "os/x11/x11.h"

#include <queue>

namespace os {

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

} // namespace os

#endif
