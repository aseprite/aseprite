// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_LOOP_H_INCLUDED
#define SHE_EVENT_LOOP_H_INCLUDED

namespace she {

  class EventLoop {
  public:
    virtual ~EventLoop() { }
    virtual void dispose() = 0;
  };

} // namespace she

#endif
