// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/x11/event_queue.h"

#include <X11/Xlib.h>

namespace she {

void X11EventQueue::getEvent(Event& ev, bool canWait)
{
  ::Display* display = X11::instance()->display();
  XSync(display, False);

  XEvent event;
  int events = XEventsQueued(display, QueuedAlready);
  for (int i=0; i<events; ++i) {
    XNextEvent(display, &event);
    processX11Event(event);
  }

  if (m_events.empty()) {
#pragma push_macro("None")
#undef None // Undefine the X11 None macro
    ev.setType(Event::None);
#pragma pop_macro("None")
  }
  else {
    ev = m_events.front();
    m_events.pop();
  }
}

void X11EventQueue::processX11Event(XEvent& event)
{
  // TODO
}

} // namespace she
