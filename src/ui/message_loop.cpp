// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

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

  if (m_manager->generateMessages()) {
    m_manager->dispatchMessages();
  }
  else {
    m_manager->collectGarbage();
  }

  // If the dispatching of messages was faster than 10 milliseconds,
  // it means that the process is not using a lot of CPU, so we can
  // wait the difference to cover those 10 milliseconds
  // sleeping. With this code we can avoid 100% CPU usage (a
  // property of Allegro 4 polling nature).
  double elapsedMSecs = chrono.elapsed() * 1000.0;
  if (elapsedMSecs > 0.0 && elapsedMSecs < 10.0)
    base::this_thread::sleep_for((10.0 - elapsedMSecs) / 1000.0);
}

} // namespace ui
