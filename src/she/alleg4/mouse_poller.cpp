// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/time.h"
#include "she/alleg4/alleg_display.h"
#include "she/display.h"
#include "she/event.h"
#include "she/event_queue.h"

#include <allegro.h>

#ifdef _WIN32
  #include <winalleg.h>
#endif

#ifdef __APPLE__
  #include <allegro/platform/aintosx.h>
#endif

namespace {

using namespace she;

#define DOUBLE_CLICK_TIMEOUT_MSECS   400

enum DlbClk {
  DOUBLE_CLICK_NONE,
  DOUBLE_CLICK_DOWN,
  DOUBLE_CLICK_UP
};

bool moved;
int she_mouse_x;
int she_mouse_y;
int she_mouse_z;
int she_mouse_b;

bool she_mouse_polling_required;

// Flag to block all the generation of mouse messages from polling.
bool mouse_left = false;

DlbClk double_click_level;
Event::MouseButton double_click_button = Event::NoneButton;
base::tick_t double_click_ticks;

#if !defined(__APPLE__) && !defined(_WIN32)
// Mouse enter/leave
int old_mouse_on = 0;
extern "C" int _mouse_on;
#endif

inline int display_w()
{
  ASSERT(unique_display);
  int scale = unique_display->scale();
  ASSERT(scale > 0);
  if (scale == 0)
    scale = 1;
  return (unique_display->width() / scale);
}

inline int display_h()
{
  ASSERT(unique_display);
  int scale = unique_display->scale();
  ASSERT(scale > 0);
  if (scale == 0)
    scale = 1;
  return (unique_display->height() / scale);
}

void update_mouse_position()
{
  she_mouse_x = display_w() * mouse_x / SCREEN_W;
  she_mouse_y = display_h() * mouse_y / SCREEN_H;
}

bool poll_mouse_position()
{
  poll_mouse();

  she_mouse_b = mouse_b;
  she_mouse_z = mouse_z;

  int old_x = she_mouse_x;
  int old_y = she_mouse_y;
  update_mouse_position();

  if ((she_mouse_x != old_x) || (she_mouse_y != old_y)) {
    // Reset double click status
    double_click_level = DOUBLE_CLICK_NONE;
    moved = true;
  }

  if (moved) {
    moved = false;
    return true;
  }
  else
    return false;
}

void generate_mouse_event_for_button(Event::MouseButton button, int old_b, int new_b)
{
  if (old_b == new_b)           // No changes in this mouse button
    return;

  Event ev;
  ev.setType(new_b ? Event::MouseDown: Event::MouseUp);

  gfx::Point mousePos(gfx::Point(she_mouse_x, she_mouse_y));
  ev.setPosition(mousePos);
  ev.setButton(button);

  // Double Click
  base::tick_t current_ticks = base::current_tick();
  if (ev.type() == Event::MouseDown) {
    if (double_click_level != DOUBLE_CLICK_NONE) {
      // Time out, back to NONE
      if ((current_ticks - double_click_ticks) > DOUBLE_CLICK_TIMEOUT_MSECS) {
        double_click_level = DOUBLE_CLICK_NONE;
      }
      else if (double_click_button == button) {
        if (double_click_level == DOUBLE_CLICK_UP) {
          ev.setType(Event::MouseDoubleClick);
        }
        else {
          double_click_level = DOUBLE_CLICK_NONE;
        }
      }
      // Press other button, back to NONE
      else {
        double_click_level = DOUBLE_CLICK_NONE;
      }
    }

    // This could be the beginning of the state
    if (double_click_level == DOUBLE_CLICK_NONE) {
      double_click_level = DOUBLE_CLICK_DOWN;
      double_click_button = button;
      double_click_ticks = current_ticks;
    }
    else {
      double_click_level = DOUBLE_CLICK_NONE;
    }
  }
  else if (ev.type() == Event::MouseUp) {
    if (double_click_level != DOUBLE_CLICK_NONE) {
      // Time out, back to NONE
      if ((current_ticks - double_click_ticks) > DOUBLE_CLICK_TIMEOUT_MSECS) {
        double_click_level = DOUBLE_CLICK_NONE;
      }
      else if (double_click_level == DOUBLE_CLICK_DOWN) {
        double_click_level = DOUBLE_CLICK_UP;
        double_click_ticks = current_ticks;
      }
    }
  }

  queue_event(ev);
}

#if __APPLE__

void osx_mouser_enter_she_callback()
{
  Event ev;
  ev.setPosition(gfx::Point(0, 0));
  ev.setType(Event::MouseEnter);
  queue_event(ev);

  mouse_left = false;
}

void osx_mouser_leave_she_callback()
{
  Event ev;
  ev.setType(Event::MouseLeave);
  queue_event(ev);

  mouse_left = true;
}

#endif // __APPLE__

void she_mouse_callback(int flags)
{
  // Avoid callbacks when the display is destroyed.
  if (!unique_display || unique_display->scale() < 1)
    return;

  update_mouse_position();
  Event ev;
  ev.setPosition(gfx::Point(she_mouse_x, she_mouse_y));

  // Mouse enter/leave for Linux
#if !defined(__APPLE__) && !defined(_WIN32)
  if (old_mouse_on != _mouse_on) {
    old_mouse_on = _mouse_on;
    ev.setType(_mouse_on ? Event::MouseEnter: Event::MouseLeave);
    queue_event(ev);
  }
#endif

  // move
  if (flags & MOUSE_FLAG_MOVE) {
    ev.setType(Event::MouseMove);
    queue_event(ev);

    // Reset double click status
    double_click_level = DOUBLE_CLICK_NONE;
  }

  // buttons
  if (flags & MOUSE_FLAG_LEFT_DOWN) {
    generate_mouse_event_for_button(Event::LeftButton, 0, 1);
  }
  if (flags & MOUSE_FLAG_LEFT_UP) {
    generate_mouse_event_for_button(Event::LeftButton, 1, 0);
  }
  if (flags & MOUSE_FLAG_RIGHT_DOWN) {
    generate_mouse_event_for_button(Event::RightButton, 0, 2);
  }
  if (flags & MOUSE_FLAG_RIGHT_UP) {
    generate_mouse_event_for_button(Event::RightButton, 2, 0);
  }
  if (flags & MOUSE_FLAG_MIDDLE_DOWN) {
    generate_mouse_event_for_button(Event::MiddleButton, 0, 4);
  }
  if (flags & MOUSE_FLAG_MIDDLE_UP) {
    generate_mouse_event_for_button(Event::MiddleButton, 4, 0);
  }

  // mouse wheel
  if (flags & MOUSE_FLAG_MOVE_Z) {
    ev.setType(Event::MouseWheel);
    ev.setWheelDelta(gfx::Point(0, int(she_mouse_z - mouse_z)));
    queue_event(ev);
    she_mouse_z = mouse_z;
  }
}

}

namespace she {

void mouse_poller_init()
{
  double_click_level = DOUBLE_CLICK_NONE;
  double_click_ticks = 0;
  moved = true;

#ifdef __APPLE__
  osx_mouse_enter_callback = osx_mouser_enter_she_callback;
  osx_mouse_leave_callback = osx_mouser_leave_she_callback;
#endif

  // optional mouse callback for supported platforms
  she_mouse_polling_required = (mouse_needs_poll() != 0);
  if (!she_mouse_polling_required)
    mouse_callback = she_mouse_callback;
}

void mouse_poller_generate_events()
{
  if (!she_mouse_polling_required)
    return;
  if (mouse_left)
    return;

  int old_b = she_mouse_b;
  int old_z = she_mouse_z;

  bool mousemove = poll_mouse_position();

  Event ev;
  gfx::Point mousePos(gfx::Point(she_mouse_x, she_mouse_y));
  ev.setPosition(mousePos);

  // Mouse movement
  if (mousemove) {
    ev.setType(Event::MouseMove);
    queue_event(ev);
  }

  // Mouse wheel
  if (old_z != she_mouse_z) {
    ev.setType(Event::MouseWheel);
    ev.setPosition(mousePos);
    ev.setWheelDelta(gfx::Point(0, old_z - she_mouse_z));
    queue_event(ev);
  }

  // Mouse clicks
  if (she_mouse_b != old_b) {
    generate_mouse_event_for_button(Event::LeftButton, old_b & 1, she_mouse_b & 1);
    generate_mouse_event_for_button(Event::RightButton, old_b & 2, she_mouse_b & 2);
    generate_mouse_event_for_button(Event::MiddleButton, old_b & 4, she_mouse_b & 4);
  }
}

} // namespace she
