// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_CLOSE_EVENT_H_INCLUDED
#define UI_CLOSE_EVENT_H_INCLUDED

#include "ui/event.h"

namespace ui {

  class CloseEvent : public Event
  {
  public:
    CloseEvent(Component* source)
      : Event(source) { }
  };

} // namespace ui

#endif  // UI_CLOSE_EVENT_H_INCLUDED
