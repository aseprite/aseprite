// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_CLOSE_EVENT_H_INCLUDED
#define UI_CLOSE_EVENT_H_INCLUDED
#pragma once

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
