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
#elif defined(ALLEGRO_UNIX)
  #include <xalleg.h>
#endif

namespace ui {

/* Global output bitmap.  */

BITMAP* ji_screen = NULL;
int ji_screen_w = 0;
int ji_screen_h = 0;

bool dirty_display_flag = true;

/* Global timer.  */

volatile int ji_clock = 0;

// Current mouse cursor type.

static CursorType mouse_cursor_type = kNoCursor;
static Cursor* mouse_cursor = NULL;
static she::Display* mouse_display = NULL;
static Overlay* mouse_cursor_overlay = NULL;

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
  ji_clock++;
}

END_OF_STATIC_FUNCTION(clock_inc);

static void set_mouse_cursor(Cursor* cursor)
{
  mouse_cursor = cursor;

  if (mouse_cursor) {
    if (!mouse_cursor_overlay) {
      mouse_cursor_overlay = new Overlay(mouse_cursor->getSurface(),
                                         gfx::Point(), Overlay::MouseZOrder);
      OverlayManager::instance()->addOverlay(mouse_cursor_overlay);
    }
    else {
      mouse_cursor_overlay->setSurface(mouse_cursor->getSurface());
      UpdateCursorOverlay();
    }
  }
  else if (mouse_cursor_overlay) {
    OverlayManager::instance()->removeOverlay(mouse_cursor_overlay);
    mouse_cursor_overlay->setSurface(NULL);
    delete mouse_cursor_overlay;
    mouse_cursor_overlay = NULL;
  }
}

int _ji_system_init()
{
  /* Install timer related stuff.  */
  LOCK_VARIABLE(ji_clock);
  LOCK_VARIABLE(m_b);
  LOCK_FUNCTION(clock_inc);

  if (install_int_ex(clock_inc, BPS_TO_TIMER(1000)) < 0)
    return -1;

  if (screen)
    jmouse_poll();

  moved = true;
  mouse_cursor_type = kNoCursor;

  return 0;
}

void _ji_system_exit()
{
  SetDisplay(NULL);
  set_mouse_cursor(NULL);

  remove_int(clock_inc);
}

void SetDisplay(she::Display* display)
{
  CursorType cursor = jmouse_get_cursor();

  jmouse_set_cursor(kNoCursor);
  mouse_display = display;
  ji_screen = (display ? reinterpret_cast<BITMAP*>(display->getSurface()->nativeHandle()): NULL);
  ji_screen_w = (ji_screen ? ji_screen->w: 0);
  ji_screen_h = (ji_screen ? ji_screen->h: 0);

  if (ji_screen != NULL) {
    Manager* manager = Manager::getDefault();
    if (manager) {
      manager->setDisplay(display);

      // Update default-manager size
      if ((manager->getBounds().w != JI_SCREEN_W ||
           manager->getBounds().h != JI_SCREEN_H)) {
        manager->setBounds(gfx::Rect(0, 0, JI_SCREEN_W, JI_SCREEN_H));
      }
    }

    jmouse_set_cursor(cursor);  // Restore mouse cursor
  }
}

void UpdateCursorOverlay()
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

CursorType jmouse_get_cursor()
{
  return mouse_cursor_type;
}

void jmouse_set_cursor(CursorType type)
{
  if (mouse_cursor_type == type)
    return;

  Theme* theme = CurrentTheme::get();
  mouse_cursor_type = type;

  if (type == kNoCursor) {
    show_mouse(NULL);
    set_mouse_cursor(NULL);
  }
  else {
    show_mouse(NULL);
    set_mouse_cursor(theme->getCursor(type));
  }

  dirty_display_flag = true;
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
    (int)JI_SCREEN_W * mouse_x / SCREEN_W,
    (int)JI_SCREEN_W * mouse_y / SCREEN_W);
}

/**
 * Updates the mouse information (position, wheel and buttons).
 *
 * @return Returns true if the mouse moved.
 */
bool jmouse_poll()
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
  m_x[1] = -1;
  m_y[1] = -1;
  m_x[0] = -1;
  m_y[0] = -1;
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

void jmouse_capture()
{
#if defined(ALLEGRO_UNIX)

  XGrabPointer(_xwin.display, _xwin.window, False,
               PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync,
               None, None, CurrentTime);

#endif
}

void jmouse_release()
{
#if defined(ALLEGRO_UNIX)

  XUngrabPointer(_xwin.display, CurrentTime);

#endif
}

MouseButtons jmouse_b(int antique)
{
  return (MouseButtons)m_b[antique & 1];
}

int jmouse_x(int antique) { return m_x[antique & 1]; }
int jmouse_y(int antique) { return m_y[antique & 1]; }
int jmouse_z(int antique) { return m_z[antique & 1]; }

gfx::Point get_delta_outside_box(const gfx::Rect& rect, const gfx::Point& mousePoint)
{
  gfx::Point delta(0, 0);

  if (mousePoint.x < rect.x)
    delta.x = mousePoint.x - rect.x;
  else if (mousePoint.x >= rect.x+rect.w)
    delta.x = mousePoint.x - (rect.x+rect.w);

  if (mousePoint.y < rect.y)
    delta.y = mousePoint.y - rect.y;
  else if (mousePoint.y > rect.y+rect.h)
    delta.y = mousePoint.y - (rect.y+rect.h);

  return delta;
}

gfx::Point control_infinite_scroll(Widget* widget, const gfx::Rect& rect, const gfx::Point& mousePoint)
{
  gfx::Point newPoint = mousePoint;
  gfx::Point delta = get_delta_outside_box(rect, newPoint);

  if (delta.x < 0) {
    newPoint.x = rect.x+rect.w+delta.x;
    if (newPoint.x < rect.x)
      newPoint.x = rect.x;
  }
  else if (delta.x > 0) {
    newPoint.x = rect.x+delta.x;
    if (newPoint.x >= rect.x+rect.w)
      newPoint.x = rect.x+rect.w-1;
  }

  if (delta.y < 0) {
    newPoint.y = rect.y+rect.h+delta.y;
    if (newPoint.y < rect.y)
      newPoint.y = rect.y;
  }
  else if (delta.y > 0) {
    newPoint.y = rect.y+delta.y;
    if (newPoint.y >= rect.y+rect.h)
      newPoint.y = rect.y+rect.h-1;
  }

  if (mousePoint != newPoint)
    ui::set_mouse_position(newPoint);

  return newPoint;
}

static void update_mouse_position(const gfx::Point& pt)
{
  m_x[0] = pt.x;
  m_y[0] = pt.y;

  if (is_windowed_mode()) {
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
            m_x[0] = JI_SCREEN_W + mouse_cursor->getFocus().x;
            m_y[0] = JI_SCREEN_H + mouse_cursor->getFocus().y;
          }
        }
        // If the mouse is captured we can put it in the edges of the
        // screen.
        else {
          pt.x -= rc.left;
          pt.y -= rc.top;

          m_x[0] = JI_SCREEN_W * pt.x / SCREEN_W;
          m_y[0] = JI_SCREEN_H * pt.y / SCREEN_H;

          m_x[0] = MID(0, m_x[0], JI_SCREEN_W-1);
          m_y[0] = MID(0, m_y[0], JI_SCREEN_H-1);
        }
      }
    }
#endif
  }
}

} // namespace ui
