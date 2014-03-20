// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_QUEUE_H_INCLUDED
#define SHE_EVENT_QUEUE_H_INCLUDED

namespace she {

  class Event;

  class EventQueue {
  public:
    virtual ~EventQueue() { }
    virtual void dispose() = 0;
    virtual void getEvent(Event& ev) = 0;
  };

} // namespace she

#endif
