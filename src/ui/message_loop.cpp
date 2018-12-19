// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/message_loop.h"

#include "base/chrono.h"
#include "base/thread.h"
#include "ui/manager.h"

namespace ui {

MessageLoop::MessageLoop(Manager* manager)
  : m_manager(manager)
{
}

void MessageLoop::pumpMessages()
{
  base::Chrono chrono;

  if (m_manager->generateMessages())
    m_manager->dispatchMessages();

  // If the dispatching of messages was faster than 10 milliseconds,
  // it means that the process is not using a lot of CPU, so we can
  // wait the difference to cover those 10 milliseconds
  // sleeping. With this code we can avoid 100% CPU usage.
  //
  // TODO check if we require this code, after adding support of event
  //      queues that wait for events.
  //
  double elapsedMSecs = chrono.elapsed() * 1000.0;
  if (elapsedMSecs > 0.0 && elapsedMSecs < 10.0)
    base::this_thread::sleep_for((10.0 - elapsedMSecs) / 1000.0);
}

} // namespace ui
