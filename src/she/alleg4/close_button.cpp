// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/alleg4/internals.h"
#include "she/event.h"
#include "she/system.h"

#include <allegro.h>

namespace {

  enum {
    STAGE_NORMAL,
    STAGE_WANT_CLOSE,
  };

  // variable to handle the external close button in some Windows enviroments
  int want_close_stage;

  // hooks the close-button in some platform with window support
  void allegro_window_close_hook()
  {
    if (want_close_stage == STAGE_NORMAL)
      want_close_stage = STAGE_WANT_CLOSE;
  }

}

namespace she {

void close_button_init()
{
  // Hook the window close message
  want_close_stage = STAGE_NORMAL;
  set_close_button_callback(allegro_window_close_hook);
}

void close_button_generate_events()
{
  // Generate Close message when the user press close button on the system window.
  if (want_close_stage == STAGE_WANT_CLOSE) {
    want_close_stage = STAGE_NORMAL;

    Event ev;
    ev.setType(Event::CloseDisplay);
    ev.setDisplay(unique_display);
    queue_event(ev);
  }
}

} // namespace she
