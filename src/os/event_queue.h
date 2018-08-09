// LAF OS Library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_EVENT_QUEUE_H_INCLUDED
#define OS_EVENT_QUEUE_H_INCLUDED
#pragma once

namespace os {

  class Event;

  class EventQueue {
  public:
    virtual ~EventQueue() { }
    virtual void getEvent(Event& ev, bool canWait) = 0;
    virtual void queueEvent(const Event& ev) = 0;

    // On MacOS X we need the EventQueue before the creation of the
    // System. E.g. when we double-click a file an Event to open that
    // file is queued in application:openFile:, code which is executed
    // before the user's main() code.
    static EventQueue* instance();
  };

  inline void queue_event(const Event& ev) {
    EventQueue::instance()->queueEvent(ev);
  }

} // namespace os

#endif
