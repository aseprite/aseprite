// LAF OS Library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_EVENT_QUEUE_INCLUDED
#define OS_WIN_EVENT_QUEUE_INCLUDED
#pragma once

#include <queue>

#include <windows.h>

#include "os/event.h"
#include "os/event_queue.h"

namespace os {

class WinEventQueue : public EventQueue {
public:
  WinEventQueue() : m_translateDeadKeys(false) {
  }

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
        // Avoid transforming WM_KEYDOWN/UP into WM_DEADCHAR/WM_CHAR
        // messages. Dead keys are converted manually in the
        // WM_KEYDOWN processing on our WinWindow<T> class.
        //
        // From MSDN TranslateMessage() documentation:
        //   "WM_KEYDOWN and WM_KEYUP combinations produce a WM_CHAR
        //   or WM_DEADCHAR message."
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms644955.aspx
        if (msg.message != WM_KEYDOWN &&
            msg.message != WM_KEYUP) {
          TranslateMessage(&msg);
        }
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

  void setTranslateDeadKeys(bool state) {
    m_translateDeadKeys = state;
  }

private:
  std::queue<Event> m_events;
  bool m_translateDeadKeys;
};

typedef WinEventQueue EventQueueImpl;

} // namespace os

#endif
