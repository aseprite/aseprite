// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/system.h"

#include "base/thread.h"
#include "gfx/point.h"
#include "os/display.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/clipboard_delegate.h"
#include "ui/cursor.h"
#include "ui/intern.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/overlay.h"
#include "ui/overlay_manager.h"
#include "ui/scale.h"
#include "ui/theme.h"
#include "ui/widget.h"

namespace ui {

// This is used to check if calls to UI layer are made from the non-UI
// thread. (Which might be catastrofic.)
base::thread::native_handle_type main_gui_thread;

// Current mouse cursor type.

static CursorType mouse_cursor_type = kOutsideDisplay;
static const Cursor* mouse_cursor_custom = nullptr;
static const Cursor* mouse_cursor = nullptr;
static os::Display* mouse_display = nullptr;
static Overlay* mouse_cursor_overlay = nullptr;
static bool use_native_mouse_cursor = true;
static bool support_native_custom_cursor = false;

// Mouse information (button and position).

static volatile MouseButtons m_buttons;
static gfx::Point m_mouse_pos;
static int mouse_cursor_scale = 1;

static int mouse_scares = 0;

static void update_mouse_overlay(const Cursor* cursor)
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

static bool update_custom_native_cursor(const Cursor* cursor)
{
  bool result = false;

  // Check if we can use a custom native mouse in this platform
  if (support_native_custom_cursor &&
      mouse_display) {
    if (cursor) {
      result = mouse_display->setNativeMouseCursor(
        // The surface is already scaled by guiscale()
        cursor->getSurface(),
        cursor->getFocus(),
        // We scale the cursor by the os::Display scale
        mouse_display->scale() * mouse_cursor_scale);
    }
    else if (mouse_cursor_type == kOutsideDisplay) {
      result = mouse_display->setNativeMouseCursor(os::kArrowCursor);
    }
    else {
      result = mouse_display->setNativeMouseCursor(os::kNoCursor);
    }
  }

  return result;
}

static void update_mouse_cursor()
{
  os::NativeCursor nativeCursor = os::kNoCursor;
  const Cursor* cursor = nullptr;

  if (use_native_mouse_cursor ||
      mouse_cursor_type == kOutsideDisplay) {
    switch (mouse_cursor_type) {
      case ui::kOutsideDisplay:
        nativeCursor = os::kArrowCursor;
        break;
      case ui::kNoCursor: break;
      case ui::kArrowCursor:
      case ui::kArrowPlusCursor:
        nativeCursor = os::kArrowCursor;
        break;
      case ui::kCrosshairCursor:
        nativeCursor = os::kCrosshairCursor;
        break;
      case ui::kForbiddenCursor:
        nativeCursor = os::kForbiddenCursor;
        break;
      case ui::kHandCursor:
        nativeCursor = os::kLinkCursor;
        break;
      case ui::kScrollCursor:
      case ui::kMoveCursor:
        nativeCursor = os::kMoveCursor;
        break;
      case ui::kSizeNSCursor: nativeCursor = os::kSizeNSCursor; break;
      case ui::kSizeWECursor: nativeCursor = os::kSizeWECursor; break;
      case ui::kSizeNCursor: nativeCursor = os::kSizeNCursor; break;
      case ui::kSizeNECursor: nativeCursor = os::kSizeNECursor; break;
      case ui::kSizeECursor: nativeCursor = os::kSizeECursor; break;
      case ui::kSizeSECursor: nativeCursor = os::kSizeSECursor; break;
      case ui::kSizeSCursor: nativeCursor = os::kSizeSCursor; break;
      case ui::kSizeSWCursor: nativeCursor = os::kSizeSWCursor; break;
      case ui::kSizeWCursor: nativeCursor = os::kSizeWCursor; break;
      case ui::kSizeNWCursor: nativeCursor = os::kSizeNWCursor; break;
    }
  }

  // Set native cursor
  if (mouse_display) {
    bool ok = mouse_display->setNativeMouseCursor(nativeCursor);

    // It looks like the specific native cursor is not supported,
    // so we can should use the internal overlay (even when we
    // have use_native_mouse_cursor flag enabled).
    if (!ok)
      nativeCursor = os::kNoCursor;
  }

  // Use a custom cursor
  if (nativeCursor == os::kNoCursor &&
      mouse_cursor_type != ui::kOutsideDisplay) {
    if (get_theme() && mouse_cursor_type != ui::kCustomCursor)
      cursor = get_theme()->getStandardCursor(mouse_cursor_type);
    else
      cursor = mouse_cursor_custom;
  }

  // Try to use a custom native cursor if it's possible
  if (nativeCursor == os::kNoCursor &&
      !update_custom_native_cursor(cursor)) {
    // Or an overlay as last resource
    update_mouse_overlay(cursor);
  }
}

static UISystem* g_instance = nullptr;

// static
UISystem* UISystem::instance()
{
  return g_instance;
}

UISystem::UISystem()
  : m_clipboardDelegate(nullptr)
{
  ASSERT(!g_instance);
  g_instance = this;

  main_gui_thread = base::this_thread::native_handle();
  mouse_cursor_type = kOutsideDisplay;
  support_native_custom_cursor =
    ((os::instance() &&
      (int(os::instance()->capabilities()) &
       int(os::Capabilities::CustomNativeMouseCursor))) ?
     true: false);

  details::initWidgets();
}

UISystem::~UISystem()
{
  OverlayManager::destroyInstance();

  // finish theme
  set_theme(nullptr, guiscale());

  details::exitWidgets();

  _internal_set_mouse_display(nullptr);
  if (!update_custom_native_cursor(nullptr))
    update_mouse_overlay(nullptr);

  ASSERT(g_instance == this);
  g_instance = nullptr;
}

void _internal_set_mouse_display(os::Display* display)
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

void set_clipboard_text(const std::string& text)
{
  ASSERT(g_instance);
  ClipboardDelegate* delegate = g_instance->clipboardDelegate();
  if (delegate)
    delegate->setClipboardText(text);
}

bool get_clipboard_text(std::string& text)
{
  ASSERT(g_instance);
  ClipboardDelegate* delegate = g_instance->clipboardDelegate();
  if (delegate)
    return delegate->getClipboardText(text);
  else
    return false;
}

void update_cursor_overlay()
{
  if (mouse_cursor_overlay != NULL && mouse_scares == 0) {
    gfx::Point newPos =
      get_mouse_position() - mouse_cursor->getFocus();

    if (newPos != mouse_cursor_overlay->position()) {
      mouse_cursor_overlay->moveOverlay(newPos);
    }
  }
}

void set_use_native_cursors(bool state)
{
  use_native_mouse_cursor = state;
  update_mouse_cursor();
}

CursorType get_mouse_cursor()
{
  return mouse_cursor_type;
}

void set_mouse_cursor(CursorType type, const Cursor* cursor)
{
  if (mouse_cursor_type == type &&
      mouse_cursor_custom == cursor)
    return;

  mouse_cursor_type = type;
  mouse_cursor_custom = cursor;
  update_mouse_cursor();
}

void set_mouse_cursor_scale(const int newScale)
{
  mouse_cursor_scale = newScale;
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
  if (!update_custom_native_cursor(nullptr))
    update_mouse_overlay(nullptr);
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

void execute_from_ui_thread(std::function<void()>&& f)
{
  ASSERT(Manager::getDefault());

  Manager* man = Manager::getDefault();
  ASSERT(man);

  FunctionMessage* msg = new FunctionMessage(std::move(f));
  msg->setRecipient(man);
  man->enqueueMessage(msg);
}

bool is_ui_thread()
{
  return (main_gui_thread == base::this_thread::native_handle());
}

#ifdef _DEBUG
void assert_ui_thread()
{
  ASSERT(is_ui_thread());
}
#endif

} // namespace ui
