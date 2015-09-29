// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/alleg4/alleg_display.h"
#include "she/display.h"
#include "she/event.h"
#include "she/event_queue.h"

#include <allegro.h>

namespace {

  bool want_close = false;
  void close_callback() {
    want_close = true;
  }

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  bool resized = false;
  int restored_width = 0;
  int restored_height = 0;

  void resize_callback(RESIZE_DISPLAY_EVENT* ev) {
    if (ev->is_maximized) {
      restored_width = ev->old_w;
      restored_height = ev->old_h;
    }
    else {
      restored_width = 0;
      restored_height = 0;
    }
    resized = true;
  }
#endif

}

namespace she {

void display_events_init()
{
  set_close_button_callback(close_callback);
#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  set_resize_callback(resize_callback);
#endif
}

void display_generate_events()
{
  if (want_close) {
    want_close = false;

    Event ev;
    ev.setType(Event::CloseDisplay);
    ev.setDisplay(unique_display);
    queue_event(ev);
  }

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  if (resized) {
    if (acknowledge_resize() < 0)
      return;

    resized = false;

    Alleg4Display* display = unique_display;
    if (display) {
      display->setOriginalWidth(restored_width);
      display->setOriginalHeight(restored_height);
      display->recreateSurface();

      Event ev;
      ev.setType(Event::ResizeDisplay);
      ev.setDisplay(display);
      queue_event(ev);
    }
  }
#endif
}

bool is_display_resize_awaiting()
{
#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  return resized;
#else
  return false;
#endif
}


} // namespace she
