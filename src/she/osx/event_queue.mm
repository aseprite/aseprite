// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

#include "she/osx/event_queue.h"

namespace she {

void OSXEventQueue::getEvent(Event& ev, bool canWait)
{
  if (!m_events.try_pop(ev)) {
    ev.setType(Event::None);
  }
}

void OSXEventQueue::queueEvent(const Event& ev)
{
  m_events.push(ev);
}

} // namespace she
