// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/system.h"

#include "gfx/point.h"
#include "she/clock.h"
#include "she/display.h"
#include "she/surface.h"
#include "ui/cursor.h"
#include "ui/intern.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/overlay.h"
#include "ui/overlay_manager.h"
#include "ui/theme.h"
#include "ui/widget.h"

namespace ui {

// Current mouse cursor type.

static CursorType mouse_cursor_type = kNoCursor;
static Cursor* mouse_cursor = NULL;
static she::Display* mouse_display = NULL;
static Overlay* mouse_cursor_overlay = NULL;
static bool use_native_mouse_cursor = false;
#ifdef USE_ALLEG4_BACKEND
  // The native cursor is hidden by default in the Allegro backend
  static bool native_cursor_set = false; // TODO remove this when we remove the Allegro backend
#else
  // The native cursor should be visible by default in
  // other "she" library backends.
  static bool native_cursor_set = true;
#endif

// Mouse information (button and position).

static volatile MouseButtons m_buttons;
static gfx::Point m_mouse_pos;

static int mouse_scares = 0;

static void update_mouse_overlay(Cursor* cursor)
{
  mouse_cursor = cursor;

  if (mouse_cursor && mouse_scares == 0) {
    if (!mouse_cursor_overlay) {
      mouse_cursor_overlay = new Overlay(
        mouse_cursor->getSurface(),
        get_mouse_position(),
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
}

UISystem::UISystem()
{
  mouse_cursor_type = kNoCursor;

  details::initWidgets();
}

UISystem::~UISystem()
{
  OverlayManager::destroyInstance();

  // finish theme
  CurrentTheme::set(NULL);

  details::exitWidgets();

  _internal_set_mouse_display(NULL);
  update_mouse_overlay(NULL);
}

int clock()
{
  return she::clock_value();
}

void _internal_set_mouse_display(she::Display* display)
{
  CursorType cursor = get_mouse_cursor();
  set_mouse_cursor(kNoCursor);
  mouse_display = display;
  if (display)
    set_mouse_cursor(cursor);  // Restore mouse cursor
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
    gfx::Point newPos =
      get_mouse_position() - mouse_cursor->getFocus();

    if (newPos != mouse_cursor_overlay->getPosition()) {
      mouse_cursor_overlay->moveOverlay(newPos);
    }
  }
}

void set_use_native_cursors(bool state)
{
  use_native_mouse_cursor = state;
}

CursorType get_mouse_cursor()
{
  return mouse_cursor_type;
}

void set_mouse_cursor(CursorType type)
{
  if (mouse_cursor_type == type)
    return;

  mouse_cursor_type = type;
  update_mouse_cursor();
}

void hide_mouse_cursor()
{
  ASSERT(mouse_scares >= 0);
  mouse_scares++;
}

void show_mouse_cursor()
{
  ASSERT(mouse_scares > 0);
  mouse_scares--;

  if (mouse_scares == 0)
    update_mouse_cursor();
}

void _internal_no_mouse_position()
{
  update_mouse_overlay(NULL);
}

void _internal_set_mouse_position(const gfx::Point& newPos)
{
  m_mouse_pos = newPos;
}

void _internal_set_mouse_buttons(MouseButtons buttons)
{
  m_buttons = buttons;
}

MouseButtons _internal_get_mouse_buttons()
{
  return m_buttons;
}

const gfx::Point& get_mouse_position()
{
  return m_mouse_pos;
}

void set_mouse_position(const gfx::Point& newPos)
{
  if (mouse_display)
    mouse_display->setMousePosition(newPos);

  _internal_set_mouse_position(newPos);
}

} // namespace ui
