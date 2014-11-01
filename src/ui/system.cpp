// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/system.h"

#include "gfx/point.h"
#include "she/display.h"
#include "she/surface.h"
#include "ui/cursor.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/overlay.h"
#include "ui/overlay_manager.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <allegro.h>
#if defined(WIN32)
  #include <winalleg.h>
#endif

namespace ui {

bool dirty_display_flag = true;

// Global timer.

static volatile int clock_var = 0;

// Current mouse cursor type.

static CursorType mouse_cursor_type = kNoCursor;
static Cursor* mouse_cursor = NULL;
static she::Display* mouse_display = NULL;
static Overlay* mouse_cursor_overlay = NULL;
static bool use_native_mouse_cursor = false;
static bool native_cursor_set = false; // If we displayed a native cursor

/* Mouse information (button and position).  */

static volatile int m_b[2];
static int m_x[2];
static int m_y[2];
static int m_z[2];

static bool moved;
static int mouse_scares = 0;

/* Local routines.  */

static void clock_inc();
static void update_mouse_position(const gfx::Point& pt);

static void clock_inc()
{
  ++clock_var;
}

END_OF_STATIC_FUNCTION(clock_inc);

static void update_mouse_overlay(Cursor* cursor)
{
  mouse_cursor = cursor;

  if (mouse_cursor && mouse_scares == 0) {
    if (!mouse_cursor_overlay) {
      mouse_cursor_overlay = new Overlay(
        mouse_cursor->getSurface(),
        gfx::Point(m_x[0], m_y[0]),
        Overlay::MouseZOrder);

      OverlayManager::instance()->addOverlay(mouse_cursor_overlay);
    }
    else {
      mouse_cursor_overlay->setSurface(mouse_cursor->getSurface());
      update_cursor_overlay();
    }
  }
  else if (mouse_cursor_overlay) {
    OverlayManager::instance()->removeOverlay(mouse_cursor_overlay);
    mouse_cursor_overlay->setSurface(NULL);
    delete mouse_cursor_overlay;
    mouse_cursor_overlay = NULL;
  }
}

static void update_mouse_cursor()
{
  show_mouse(NULL);

  // Use native cursor when it's possible/available/configured to do so.

  bool native_cursor_available = false;
  if (use_native_mouse_cursor || mouse_cursor_type == kOutsideDisplay) {
    she::NativeCursor nativeCursor = she::kNoCursor;

    native_cursor_available = true;
    switch (mouse_cursor_type) {
      case ui::kOutsideDisplay:
        nativeCursor = she::kArrowCursor;
        break;
      case ui::kNoCursor: break;
      case ui::kArrowCursor:
      case ui::kArrowPlusCursor:
        nativeCursor = she::kArrowCursor;
        break;
      case ui::kForbiddenCursor:
        nativeCursor = she::kForbiddenCursor;
        break;
      case ui::kHandCursor:
        nativeCursor = she::kLinkCursor;
        break;
      case ui::kScrollCursor:
      case ui::kMoveCursor:
        nativeCursor = she::kMoveCursor;
        break;
      case ui::kSizeNSCursor: nativeCursor = she::kSizeNSCursor; break;
      case ui::kSizeWECursor: nativeCursor = she::kSizeWECursor; break;
      case ui::kSizeNCursor: nativeCursor = she::kSizeNCursor; break;
      case ui::kSizeNECursor: nativeCursor = she::kSizeNECursor; break;
      case ui::kSizeECursor: nativeCursor = she::kSizeECursor; break;
      case ui::kSizeSECursor: nativeCursor = she::kSizeSECursor; break;
      case ui::kSizeSCursor: nativeCursor = she::kSizeSCursor; break;
      case ui::kSizeSWCursor: nativeCursor = she::kSizeSWCursor; break;
      case ui::kSizeWCursor: nativeCursor = she::kSizeWCursor; break;
      case ui::kSizeNWCursor: nativeCursor = she::kSizeNWCursor; break;
      default:
        native_cursor_available = false;
        break;
    }

    if (native_cursor_available) {
      native_cursor_available =
        mouse_display->setNativeMouseCursor(nativeCursor);
      native_cursor_set = (nativeCursor != she::kNoCursor);
    }
  }

  // Hide native cursor if it is visible but the current cursor type
  // is not supported natively.

  if (!native_cursor_available && native_cursor_set) {
    mouse_display->setNativeMouseCursor(she::kNoCursor);
    native_cursor_set = false;
  }

  // Use a software cursor with the overlay.

  if (!native_cursor_set) {
    if (mouse_cursor_type == ui::kNoCursor) {
      update_mouse_overlay(NULL);
    }
    else {
      update_mouse_overlay(CurrentTheme::get()->getCursor(mouse_cursor_type));
    }
  }
  else {
    // Hide the overlay if we are using a native cursor.
    update_mouse_overlay(NULL);
  }

  dirty_display_flag = true;
}

int _ji_system_init()
{
  /* Install timer related stuff.  */
  LOCK_VARIABLE(clock_var);
  LOCK_VARIABLE(m_b);
  LOCK_FUNCTION(clock_inc);

  if (install_int_ex(clock_inc, BPS_TO_TIMER(1000)) < 0)
    return -1;

  if (screen)
    _internal_poll_mouse();

  moved = true;
  mouse_cursor_type = kNoCursor;

  return 0;
}

void _ji_system_exit()
{
  set_display(NULL);
  update_mouse_overlay(NULL);

  remove_int(clock_inc);
}

int clock()
{
  return clock_var;
}

void set_display(she::Display* display)
{
  CursorType cursor = jmouse_get_cursor();

  jmouse_set_cursor(kNoCursor);
  mouse_display = display;

  if (display) {
    Manager* manager = Manager::getDefault();
    if (manager) {
      manager->setDisplay(display);

      // Update default-manager size
      if ((manager->getBounds().w != ui::display_w() ||
           manager->getBounds().h != ui::display_h())) {
        manager->setBounds(gfx::Rect(0, 0, ui::display_w(), ui::display_h()));
      }
    }

    jmouse_set_cursor(cursor);  // Restore mouse cursor
  }
}

int display_w()
{
  if (mouse_display)
    return mouse_display->width() / mouse_display->scale();
  else
    return 1;
}

int display_h()
{
  if (mouse_display)
    return mouse_display->height() / mouse_display->scale();
  else
    return 1;
}

void update_cursor_overlay()
{
  if (mouse_cursor_overlay != NULL && mouse_scares == 0) {
    gfx::Point newPos(m_x[0]-mouse_cursor->getFocus().x,
                      m_y[0]-mouse_cursor->getFocus().y);

    if (newPos != mouse_cursor_overlay->getPosition()) {
      mouse_cursor_overlay->moveOverlay(newPos);
      dirty_display_flag = true;
    }
  }
}

void set_use_native_cursors(bool state)
{
  use_native_mouse_cursor = state;
}

CursorType jmouse_get_cursor()
{
  return mouse_cursor_type;
}

void jmouse_set_cursor(CursorType type)
{
  if (mouse_cursor_type == type)
    return;

  mouse_cursor_type = type;
  update_mouse_cursor();
}

void jmouse_hide()
{
  ASSERT(mouse_scares >= 0);
  mouse_scares++;
}

void jmouse_show()
{
  ASSERT(mouse_scares > 0);
  mouse_scares--;

  if (mouse_scares == 0)
    update_mouse_cursor();
}

bool jmouse_is_hidden()
{
  ASSERT(mouse_scares >= 0);
  return mouse_scares > 0;
}

bool jmouse_is_shown()
{
  ASSERT(mouse_scares >= 0);
  return mouse_scares == 0;
}

static gfx::Point allegro_mouse_point()
{
  return gfx::Point(
    (int)ui::display_w() * mouse_x / SCREEN_W,
    (int)ui::display_w() * mouse_y / SCREEN_W);
}

/**
 * Updates the mouse information (position, wheel and buttons).
 *
 * @return Returns true if the mouse moved.
 */
bool _internal_poll_mouse()
{
  m_b[1] = m_b[0];
  m_x[1] = m_x[0];
  m_y[1] = m_y[0];
  m_z[1] = m_z[0];

  poll_mouse();

  m_b[0] = mouse_b;
  m_z[0] = mouse_z;

  update_mouse_position(allegro_mouse_point());

  if ((m_x[0] != m_x[1]) || (m_y[0] != m_y[1])) {
    poll_mouse();
    update_mouse_position(allegro_mouse_point());
    moved = true;
  }

  if (moved) {
    moved = false;
    return true;
  }
  else
    return false;
}

void _internal_no_mouse_position()
{
  moved = true;
  update_mouse_overlay(NULL);
}

void _internal_set_mouse_position(const gfx::Point& newPos)
{
  moved = true;
  if (m_x[0] >= 0) {
    m_x[1] = m_x[0];
    m_y[1] = m_y[0];
  }
  else {
    m_x[1] = newPos.x;
    m_y[1] = newPos.y;
  }
  m_x[0] = newPos.x;
  m_y[0] = newPos.y;
}

void _internal_set_mouse_buttons(MouseButtons buttons)
{
  m_b[1] = m_b[0];
  m_b[0] = buttons;
}

gfx::Point get_mouse_position()
{
  return gfx::Point(jmouse_x(0), jmouse_y(0));
}

void set_mouse_position(const gfx::Point& newPos)
{
  moved = true;

  if (mouse_display)
    mouse_display->setMousePosition(newPos);

  update_mouse_position(newPos);

  m_x[1] = m_x[0];
  m_y[1] = m_y[0];
}

MouseButtons jmouse_b(int antique)
{
  return (MouseButtons)m_b[antique & 1];
}

int jmouse_x(int antique) { return m_x[antique & 1]; }
int jmouse_y(int antique) { return m_y[antique & 1]; }
int jmouse_z(int antique) { return m_z[antique & 1]; }

static void update_mouse_position(const gfx::Point& pt)
{
  m_x[0] = pt.x;
  m_y[0] = pt.y;

  if (is_windowed_mode()) {     // TODO move this to "she" module
#ifdef WIN32
    // This help us (on Windows) to get mouse feedback when we capture
    // the mouse but we are outside the window.
    POINT pt;
    RECT rc;

    if (GetCursorPos(&pt) && GetClientRect(win_get_window(), &rc)) {
      MapWindowPoints(win_get_window(), NULL, (LPPOINT)&rc, 2);

      if (!PtInRect(&rc, pt)) {
        // If the mouse is free we can hide the cursor putting the
        // mouse outside the screen (right-bottom corder).
        if (!Manager::getDefault()->getCapture()) {
          if (mouse_cursor) {
            m_x[0] = ui::display_w() + mouse_cursor->getFocus().x;
            m_y[0] = ui::display_h() + mouse_cursor->getFocus().y;
          }
        }
        // If the mouse is captured we can put it in the edges of the
        // screen.
        else {
          pt.x -= rc.left;
          pt.y -= rc.top;

          m_x[0] = ui::display_w() * pt.x / SCREEN_W;
          m_y[0] = ui::display_h() * pt.y / SCREEN_H;

          m_x[0] = MID(0, m_x[0], ui::display_w()-1);
          m_y[0] = MID(0, m_y[0], ui::display_h()-1);
        }
      }
    }
#endif
  }
}

} // namespace ui
