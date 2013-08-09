// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_SAVE_LAYOUT_EVENT_H_INCLUDED
#define UI_SAVE_LAYOUT_EVENT_H_INCLUDED

#include "ui/event.h"
#include <iosfwd>

namespace ui {

  class Widget;

  class SaveLayoutEvent : public Event
  {
  public:
    SaveLayoutEvent(Widget* source, std::ostream& stream)
      : Event(source)
      , m_stream(stream) {
    }

    std::ostream& stream() { return m_stream; }

  private:
    std::ostream& m_stream;
  };

} // namespace ui

#endif
