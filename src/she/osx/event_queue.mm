// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "she/osx/event_queue.h"

namespace she {

static NSWindow* g_window = nil;

void OSXEventQueue::getEvent(Event& ev, bool canWait)
{
  ev.setType(Event::None);

retry:;
  NSApplication* app = [NSApplication sharedApplication];
  if (!app)
    return;

  // Pump the whole queue of Cocoa events
  NSEvent* event;
  do {
    event = [app nextEventMatchingMask:NSAnyEventMask
                             untilDate:[NSDate distantPast]
                                inMode:NSDefaultRunLoopMode
                               dequeue:YES];
    if (event)
      [app sendEvent: event];
  } while (event);

  if (!m_events.try_pop(ev)) {
    if (canWait) {
      // Wait until there is a Cocoa event in queue
      [NSApp nextEventMatchingMask:NSAnyEventMask
                         untilDate:[NSDate distantFuture]
                            inMode:NSDefaultRunLoopMode
                           dequeue:NO];
      goto retry;
    }
  }
}

void OSXEventQueue::queueEvent(const Event& ev)
{
  m_events.push(ev);
}

} // namespace she
