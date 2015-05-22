// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_QUEUE_H_INCLUDED
#define SHE_EVENT_QUEUE_H_INCLUDED
#pragma once

namespace she {

  class Event;

  class EventQueue {
  public:
    virtual ~EventQueue() { }
    virtual void getEvent(Event& ev, bool canWait) = 0;
    virtual void queueEvent(const Event& ev) = 0;
  };

} // namespace she

#endif
