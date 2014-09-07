// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/alleg4/internals.h"
#include "she/clock.h"
#include "she/display.h"
#include "she/event.h"

#include <allegro.h>
#ifdef WIN32
  #include <winalleg.h>
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

DlbClk double_click_level;
Event::MouseButton double_click_button = Event::NoneButton;
int double_click_ticks;

inline int display_w() { return (unique_display->width() / unique_display->scale()); }
inline int display_h() { return (unique_display->height() / unique_display->scale()); }

gfx::Point allegro_mouse_point()
{
  return gfx::Point(
    (int)display_w() * mouse_x / SCREEN_W,
    (int)display_h() * mouse_y / SCREEN_H);
}

void update_mouse_position(const gfx::Point& pt)
{
  she_mouse_x = pt.x;
  she_mouse_y = pt.y;

#ifdef WIN32
  if (is_windowed_mode()) {
    // This help us (on Windows) to get mouse feedback when we capture
    // the mouse but we are outside the window.
    POINT pt;
    RECT rc;

    if (GetCursorPos(&pt) && GetClientRect(win_get_window(), &rc)) {
      MapWindowPoints(win_get_window(), NULL, (LPPOINT)&rc, 2);

      if (!PtInRect(&rc, pt)) {
        // If the mouse is free we can hide the cursor putting the
        // mouse outside the screen (right-bottom corder).
        if (GetCapture() != win_get_window()) {
          she_mouse_x = display_w() + 32 * unique_display->scale();
          she_mouse_y = display_h() + 32 * unique_display->scale();
        }
        // If the mouse is captured we can put it in the edges of the
        // screen.
        else {
          pt.x -= rc.left;
          pt.y -= rc.top;

          she_mouse_x = display_w() * pt.x / SCREEN_W;
          she_mouse_y = display_h() * pt.y / SCREEN_H;
          she_mouse_x = MID(0, she_mouse_x, display_w()-1);
          she_mouse_y = MID(0, she_mouse_y, display_h()-1);
        }
      }
    }
  }
#endif
}

bool poll_mouse_position()
{
  poll_mouse();

  int old_x = she_mouse_x;
  int old_y = she_mouse_y;

  she_mouse_x = mouse_x;
  she_mouse_y = mouse_y;
  she_mouse_b = mouse_b;
  she_mouse_z = mouse_z;

  update_mouse_position(allegro_mouse_point());

  if ((she_mouse_x != old_x) || (she_mouse_y != old_y)) {
    poll_mouse();
    update_mouse_position(allegro_mouse_point());

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
  int current_ticks = clock_value();
  if (ev.type() == Event::MouseDown) {
    if (double_click_level != DOUBLE_CLICK_NONE) {
      // Time out, back to NONE
      if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
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
  }
  else if (ev.type() == Event::MouseUp) {
    if (double_click_level != DOUBLE_CLICK_NONE) {
      // Time out, back to NONE
      if (current_ticks - double_click_ticks > DOUBLE_CLICK_TIMEOUT_MSECS) {
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

}

namespace she { 

void mouse_poller_init()
{
  double_click_level = DOUBLE_CLICK_NONE;
  double_click_ticks = 0;
  moved = true;
}

void mouse_poller_generate_events()
{
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

