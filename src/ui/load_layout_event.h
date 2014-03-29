// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_LOAD_LAYOUT_EVENT_H_INCLUDED
#define UI_LOAD_LAYOUT_EVENT_H_INCLUDED
#pragma once

#include "ui/event.h"
#include <iosfwd>

namespace ui {

  class Widget;

  class LoadLayoutEvent : public Event
  {
  public:
    LoadLayoutEvent(Widget* source, std::istream& stream)
      : Event(source)
      , m_stream(stream) {
    }

    std::istream& stream() { return m_stream; }

  private:
    std::istream& m_stream;
  };

} // namespace ui

#endif
