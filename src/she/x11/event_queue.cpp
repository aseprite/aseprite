// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/x11/event_queue.h"

#include "she/x11/window.h"

#include <X11/Xlib.h>

namespace she {

namespace {

const char* get_event_name(XEvent& event)
{
  switch (event.type) {
    case KeyPress: return "KeyPress";
    case KeyRelease: return "KeyRelease";
    case ButtonPress: return "ButtonPress";
    case ButtonRelease: return "ButtonRelease";
    case MotionNotify: return "MotionNotify";
    case EnterNotify: return "EnterNotify";
    case LeaveNotify: return "LeaveNotify";
    case FocusIn: return "FocusIn";
    case FocusOut: return "FocusOut";
    case KeymapNotify: return "KeymapNotify";
    case Expose: return "Expose";
    case GraphicsExpose: return "GraphicsExpose";
    case NoExpose: return "NoExpose";
    case VisibilityNotify: return "VisibilityNotify";
    case CreateNotify: return "CreateNotify";
    case DestroyNotify: return "DestroyNotify";
    case UnmapNotify: return "UnmapNotify";
    case MapNotify: return "MapNotify";
    case MapRequest: return "MapRequest";
    case ReparentNotify: return "ReparentNotify";
    case ConfigureNotify: return "ConfigureNotify";
    case ConfigureRequest: return "ConfigureRequest";
    case GravityNotify: return "GravityNotify";
    case ResizeRequest: return "ResizeRequest";
    case CirculateNotify: return "CirculateNotify";
    case CirculateRequest: return "CirculateRequest";
    case PropertyNotify: return "PropertyNotify";
    case SelectionClear: return "SelectionClear";
    case SelectionRequest: return "SelectionRequest";
    case SelectionNotify: return "SelectionNotify";
    case ColormapNotify: return "ColormapNotify";
    case ClientMessage: return "ClientMessage";
    case MappingNotify: return "MappingNotify";
    case GenericEvent: return "GenericEvent";
  }
  return "Unknown";
}

} // anonymous namespace

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
  TRACE("XEvent: %s (%d)\n", get_event_name(event), event.type);

  X11Window* window = X11Window::getPointerFromHandle(event.xany.window);
  // In MappingNotify the window can be nullptr
  if (window)
    window->processX11Event(event);
}

} // namespace she
