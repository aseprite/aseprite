// SHE library
// Copyright (C) 2015-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "she/osx/event_queue.h"

namespace she {

void OSXEventQueue::getEvent(Event& ev, bool canWait)
{
  ev.setType(Event::None);

  @autoreleasepool {
  retry:;
    NSApplication* app = [NSApplication sharedApplication];
    if (!app)
      return;

    // Pump the whole queue of Cocoa events
    NSEvent* event;
    do {
      event = [app nextEventMatchingMask:NSEventMaskAny
                               untilDate:[NSDate distantPast]
                                  inMode:NSDefaultRunLoopMode
                                 dequeue:YES];
      if (event) {
        // Intercept <Control+Tab>, <Cmd+[>, and other keyboard
        // combinations, and send them directly to the main
        // NSView. Without this, the NSApplication intercepts the key
        // combination and use it to go to the next key view.
        if (event.type == NSEventTypeKeyDown) {
          [app.mainWindow.contentView keyDown:event];
        }
        else {
          [app sendEvent:event];
        }
      }
    } while (event);

    if (!m_events.try_pop(ev)) {
      if (canWait) {
        // Wait until there is a Cocoa event in queue
        [NSApp nextEventMatchingMask:NSEventMaskAny
                           untilDate:[NSDate distantFuture]
                              inMode:NSDefaultRunLoopMode
                             dequeue:NO];
        goto retry;
      }
    }
  }
}

void OSXEventQueue::queueEvent(const Event& ev)
{
  m_events.push(ev);
}

} // namespace she
