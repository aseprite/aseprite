// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_CLOSE_EVENT_H_INCLUDED
#define GUI_CLOSE_EVENT_H_INCLUDED

#include "gui/event.h"

class CloseEvent : public Event
{
public:
  CloseEvent(Component* source)
    : Event(source) { }
};

#endif  // GUI_CLOSE_EVENT_H_INCLUDED
