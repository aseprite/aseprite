// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_EVENT_QUEUE_INCLUDED
#define SHE_WIN_EVENT_QUEUE_INCLUDED
#pragma once

#include <queue>

#include <windows.h>

#include "she/event.h"
#include "she/event_queue.h"

namespace she {

class WinEventQueue : public EventQueue {
public:
  void getEvent(Event& ev, bool canWait) override {
    MSG msg;

    while (m_events.empty()) {
      BOOL res;

      if (canWait) {
        ASSERT(false);          // Not yet supported
        res = GetMessage(&msg, nullptr, 0, 0);
      }
      else {
        res = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
      }

      if (res) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else if (!canWait)
        break;
    }

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

typedef WinEventQueue EventQueueImpl;

} // namespace she

#endif
