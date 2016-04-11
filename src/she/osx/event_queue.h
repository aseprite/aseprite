// SHE library
// Copyright (C) 2015-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_EVENT_QUEUE_INCLUDED
#define SHE_OSX_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "she/event.h"
#include "she/event_queue.h"

namespace she {

class OSXEventQueue : public EventQueue {
public:
  void getEvent(Event& ev, bool canWait) override;
  void queueEvent(const Event& ev) override;

private:
  base::concurrent_queue<Event> m_events;
};

typedef OSXEventQueue EventQueueImpl;

} // namespace she

#endif
