// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/system.h"

#include "gfx/point.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/clipboard_delegate.h"
#include "ui/cursor.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/overlay.h"
#include "ui/overlay_manager.h"
#include "ui/scale.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <thread>

namespace ui {

// This is used to check if calls to UI layer are made from the non-UI
// thread. (Which might be catastrofic.)
std::thread::id main_gui_thread;

// Multiple displays (create one os::Window for each ui::Window)
bool multi_displays = false;

// Current mouse cursor type.
static CursorType mouse_cursor_type = kOutsideDisplay;
static const Cursor* mouse_cursor_custom = nullptr;
static const Cursor* mouse_cursor = nullptr;
static Display* mouse_display = nullptr;
static OverlayRef mouse_cursor_overlay = nullptr;
static bool use_native_mouse_cursor = true;
static bool support_native_custom_cursor = false;

// Mouse information
static int mouse_cursor_scale = 1;
static int mouse_scares = 0;

static void update_mouse_overlay(const Cursor* cursor)
{
  mouse_cursor = cursor;

  if (mouse_cursor && mouse_scares == 0) {
    if (!mouse_cursor_overlay) {
      ASSERT(mouse_display);
      mouse_cursor_overlay = base::make_ref<Overlay>(
        mouse_display,
        mouse_cursor->surface(),
        mouse_display->nativeWindow()->pointFromScreen(get_mouse_position()),
        Overlay::MouseZOrder);

      OverlayManager::instance()->addOverlay(mouse_cursor_overlay);
    }
    else {
      mouse_cursor_overlay->setSurface(mouse_cursor->surface());
      update_cursor_overlay();
    }
  }
  else if (mouse_cursor_overlay) {
    OverlayManager::instance()->removeOverlay(mouse_cursor_overlay);
    mouse_cursor_overlay->setSurface(nullptr);
    mouse_cursor_overlay.reset();
  }
}

static bool set_native_cursor_on_all_displays(Display* display,
                                              const Cursor* cursor)
{
  bool result = false;
  while (display) {
    os::Window* nativeWindow = display->nativeWindow();

    if (cursor && cursor->surface()) {
      // The cursor surface is already scaled by guiscale(), we scale
      // the cursor by the os::Window scale and mouse scale.
      const int scale = nativeWindow->scale() * mouse_cursor_scale;
      if (auto osCursor = cursor->nativeCursor(scale))
        result |= nativeWindow->setCursor(osCursor);
    }
    else if (mouse_cursor_type == kOutsideDisplay) {
      result |= nativeWindow->setCursor(os::NativeCursor::Arrow);
    }
    else {
      result |= nativeWindow->setCursor(os::NativeCursor::Hidden);
    }
    display = display->parentDisplay();
  }
  return result;
}

static bool set_native_cursor_on_all_displays(Display* display,
                                              const os::NativeCursor cursor)
{
  bool result = false;
  while (display) {
    os::Window* nativeWindow = display->nativeWindow();
    if (mouse_cursor_type == kOutsideDisplay) {
      result |= nativeWindow->setCursor(os::NativeCursor::Arrow);
    }
    else {
      result |= nativeWindow->setCursor(cursor);
    }
    display = display->parentDisplay();
  }
  return result;
}

static bool update_custom_native_cursor(const Cursor* cursor)
{
  bool result = false;

  // Check if we can use a custom native mouse in this platform
  if (support_native_custom_cursor &&
      mouse_display) {
    result = set_native_cursor_on_all_displays(mouse_display, cursor);
  }

  return result;
}

static void update_mouse_cursor()
{
  os::NativeCursor nativeCursor = os::NativeCursor::Hidden;
  const Cursor* cursor = nullptr;

  if (use_native_mouse_cursor ||
      mouse_cursor_type == kOutsideDisplay) {
    switch (mouse_cursor_type) {
      case ui::kOutsideDisplay:
        nativeCursor = os::NativeCursor::Arrow;
        break;
      case ui::kNoCursor: break;
      case ui::kArrowCursor:
      case ui::kArrowPlusCursor:
        nativeCursor = os::NativeCursor::Arrow;
        break;
      case ui::kCrosshairCursor:
        nativeCursor = os::NativeCursor::Crosshair;
        break;
      case ui::kForbiddenCursor:
        nativeCursor = os::NativeCursor::Forbidden;
        break;
      case ui::kHandCursor:
        nativeCursor = os::NativeCursor::Link;
        break;
      case ui::kScrollCursor:
      case ui::kMoveCursor:
        nativeCursor = os::NativeCursor::Move;
        break;
      case ui::kSizeNSCursor: nativeCursor = os::NativeCursor::SizeNS; break;
      case ui::kSizeWECursor: nativeCursor = os::NativeCursor::SizeWE; break;
      case ui::kSizeNCursor: nativeCursor = os::NativeCursor::SizeN; break;
      case ui::kSizeNECursor: nativeCursor = os::NativeCursor::SizeNE; break;
      case ui::kSizeECursor: nativeCursor = os::NativeCursor::SizeE; break;
      case ui::kSizeSECursor: nativeCursor = os::NativeCursor::SizeSE; break;
      case ui::kSizeSCursor: nativeCursor = os::NativeCursor::SizeS; break;
      case ui::kSizeSWCursor: nativeCursor = os::NativeCursor::SizeSW; break;
      case ui::kSizeWCursor: nativeCursor = os::NativeCursor::SizeW; break;
      case ui::kSizeNWCursor: nativeCursor = os::NativeCursor::SizeNW; break;
    }
  }

  // Set native cursor
  if (mouse_display) {
    bool ok = set_native_cursor_on_all_displays(mouse_display, nativeCursor);

    // It looks like the specific native cursor is not supported,
    // so we can should use the internal overlay (even when we
    // have use_native_mouse_cursor flag enabled).
    if (!ok)
      nativeCursor = os::NativeCursor::Hidden;
  }

  // Use a custom cursor
  if (nativeCursor == os::NativeCursor::Hidden &&
      mouse_cursor_type != ui::kOutsideDisplay) {
    if (get_theme() && mouse_cursor_type != ui::kCustomCursor)
      cursor = get_theme()->getStandardCursor(mouse_cursor_type);
    else
      cursor = mouse_cursor_custom;
  }

  // Try to use a custom native cursor if it's possible
  if (mouse_display &&
      nativeCursor == os::NativeCursor::Hidden &&
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

  main_gui_thread = std::this_thread::get_id();
  mouse_cursor_type = kOutsideDisplay;
  support_native_custom_cursor =
    ((os::instance() &&
      (int(os::instance()->capabilities()) &
       int(os::Capabilities::CustomMouseCursor))) ?
     true: false);

  details::initWidgets();
}

UISystem::~UISystem()
{
  // finish theme
  set_theme(nullptr, guiscale());

  details::exitWidgets();

  _internal_set_mouse_display(nullptr);
  if (!update_custom_native_cursor(nullptr))
    update_mouse_overlay(nullptr);

  OverlayManager::destroyInstance();

  ASSERT(g_instance == this);
  g_instance = nullptr;
}

void _internal_set_mouse_display(Display* display)
{
  if (display != mouse_display) {
    mouse_display = display;
    if (mouse_display)
      update_mouse_cursor();
  }
}

void set_multiple_displays(bool multi)
{
  multi_displays = multi;
}

bool get_multiple_displays()
{
  return multi_displays;
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
  if (mouse_cursor_overlay != nullptr && mouse_scares == 0) {
    gfx::Point newPos =
      mouse_display->nativeWindow()->pointFromScreen(get_mouse_position())
      - mouse_cursor->focus();

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

void set_mouse_cursor_reset_info()
{
  mouse_cursor_type = kCustomCursor;
  mouse_cursor_custom = nullptr;
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

gfx::Point get_mouse_position()
{
  return os::instance()->mousePosition();
}

void set_mouse_position(const gfx::Point& newPos,
                        Display* display)
{
  if (display && display != mouse_display)
    _internal_set_mouse_display(display);

  if (display)
    display->nativeWindow()->setMousePosition(newPos);
  else
    os::instance()->setMousePosition(newPos);
}

void execute_from_ui_thread(std::function<void()>&& func)
{
  // Queue the event
  os::Event ev;
  ev.setType(os::Event::Callback);
  ev.setCallback(std::move(func));
  os::queue_event(ev);
}

bool is_ui_thread()
{
  return (main_gui_thread == std::this_thread::get_id());
}

#ifdef _DEBUG
void assert_ui_thread()
{
  ASSERT(is_ui_thread());
}
#endif

} // namespace ui
