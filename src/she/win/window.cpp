// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/window.h"

#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <sstream>

#include "base/base.h"
#include "base/debug.h"
#include "base/log.h"
#include "gfx/size.h"
#include "she/event.h"
#include "she/native_cursor.h"
#include "she/win/system.h"
#include "she/win/vk.h"
#include "she/win/window_dde.h"

#define SHE_WND_CLASS_NAME L"Aseprite.Window"

#define KEY_TRACE TRACE
#define MOUSE_TRACE(...)

// Gets the window client are in absolute/screen coordinates
#define ABS_CLIENT_RC(rc)                               \
  RECT rc;                                              \
  GetClientRect(m_hwnd, &rc);                           \
  MapWindowPoints(m_hwnd, NULL, (POINT*)&rc, 2)

// Not yet ready because if we start receiving WM_POINTERDOWN messages
// instead of WM_LBUTTONDBLCLK we lost the automatic double-click
// messages.
#define USE_EnableMouseInPointer 0

#ifndef INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN
#define INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN 1
#endif

namespace she {

WinWindow::WinWindow(int width, int height, int scale)
  : m_hwnd(nullptr)
  , m_hcursor(nullptr)
  , m_clientSize(1, 1)
  , m_restoredSize(0, 0)
  , m_scale(scale)
  , m_isCreated(false)
  , m_translateDeadKeys(false)
  , m_hasMouse(false)
  , m_captureMouse(false)
  , m_customHcursor(false)
  , m_usePointerApi(false)
  , m_ignoreMouseMessages(false)
  , m_lastPointerId(0)
  , m_ictx(nullptr)
  , m_hpenctx(nullptr)
  , m_pointerType(PointerType::Unknown)
  , m_pressure(0.0)
{
  auto& winApi = system()->winApi();
  if (winApi.EnableMouseInPointer &&
      winApi.IsMouseInPointerEnabled &&
      winApi.GetPointerInfo &&
      winApi.GetPointerPenInfo) {
#if USE_EnableMouseInPointer == 1
    if (!winApi.IsMouseInPointerEnabled()) {
      // Prefer pointer messages (WM_POINTER*) since Windows 8 instead
      // of mouse messages (WM_MOUSE*)
      winApi.EnableMouseInPointer(TRUE);
      m_ignoreMouseMessages = (winApi.IsMouseInPointerEnabled() ? true: false);
    }
#endif

    // Initialize a Interaction Context to convert WM_POINTER messages
    // into gestures processed by handleInteractionContextOutput().
    if (winApi.CreateInteractionContext &&
        winApi.RegisterOutputCallbackInteractionContext &&
        winApi.SetInteractionConfigurationInteractionContext) {
      HRESULT hr = winApi.CreateInteractionContext(&m_ictx);
      if (SUCCEEDED(hr)) {
        hr = winApi.RegisterOutputCallbackInteractionContext(
          m_ictx, &WinWindow::staticInteractionContextCallback, this);
      }
      if (SUCCEEDED(hr)) {
        INTERACTION_CONTEXT_CONFIGURATION cfg[] = {
          { INTERACTION_ID_MANIPULATION,
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_INERTIA |
            INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING_INERTIA },
          { INTERACTION_ID_TAP,
            INTERACTION_CONFIGURATION_FLAG_TAP |
            INTERACTION_CONFIGURATION_FLAG_TAP_DOUBLE },
          { INTERACTION_ID_SECONDARY_TAP,
            INTERACTION_CONFIGURATION_FLAG_SECONDARY_TAP },
          { INTERACTION_ID_HOLD,
            INTERACTION_CONFIGURATION_FLAG_NONE },
          { INTERACTION_ID_DRAG,
            INTERACTION_CONFIGURATION_FLAG_NONE },
          { INTERACTION_ID_CROSS_SLIDE,
            INTERACTION_CONFIGURATION_FLAG_NONE }
        };
        hr = winApi.SetInteractionConfigurationInteractionContext(
          m_ictx, sizeof(cfg) / sizeof(INTERACTION_CONTEXT_CONFIGURATION), cfg);
      }
      if (SUCCEEDED(hr)) {
        hr = winApi.SetPropertyInteractionContext(
          m_ictx,
          INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS,
          INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN);
      }
    }

    m_usePointerApi = true;
  }

  registerClass();

  // The HWND returned by CreateWindowEx() is different than the
  // HWND used in WM_CREATE message.
  m_hwnd = createHwnd(this, width, height);
  if (!m_hwnd)
    throw std::runtime_error("Error creating window");

  SetWindowLongPtr(m_hwnd, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));

  // This flag is used to avoid calling T::resizeImpl() when we
  // add the scrollbars to the window. (As the T type could not be
  // fully initialized yet.)
  m_isCreated = true;
}

WinWindow::~WinWindow()
{
  auto& winApi = system()->winApi();
  if (m_ictx && winApi.DestroyInteractionContext)
    winApi.DestroyInteractionContext(m_ictx);

  if (m_hwnd)
    DestroyWindow(m_hwnd);
}

void WinWindow::queueEvent(Event& ev)
{
  onQueueEvent(ev);
}

void WinWindow::setScale(int scale)
{
  m_scale = scale;
  onResize(m_clientSize);
}

void WinWindow::setVisible(bool visible)
{
  if (visible) {
    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    UpdateWindow(m_hwnd);
    DrawMenuBar(m_hwnd);
  }
  else
    ShowWindow(m_hwnd, SW_HIDE);
}

void WinWindow::maximize()
{
  ShowWindow(m_hwnd, SW_MAXIMIZE);
}

bool WinWindow::isMaximized() const
{
  return (IsZoomed(m_hwnd) ? true: false);
}

bool WinWindow::isMinimized() const
{
  return (GetWindowLong(m_hwnd, GWL_STYLE) & WS_MINIMIZE ? true: false);
}

gfx::Size WinWindow::clientSize() const
{
  return m_clientSize;
}

gfx::Size WinWindow::restoredSize() const
{
  return m_restoredSize;
}

void WinWindow::setTitle(const std::string& title)
{
  SetWindowText(m_hwnd, base::from_utf8(title).c_str());
}

void WinWindow::captureMouse()
{
  m_captureMouse = true;

  if (GetCapture() != m_hwnd) {
    MOUSE_TRACE("SetCapture\n");
    SetCapture(m_hwnd);
  }
}

void WinWindow::releaseMouse()
{
  m_captureMouse = false;

  if (GetCapture() == m_hwnd) {
    MOUSE_TRACE("ReleaseCapture\n");
    ReleaseCapture();
  }
}

void WinWindow::setMousePosition(const gfx::Point& position)
{
  POINT pos = { position.x * m_scale,
                position.y * m_scale };
  ClientToScreen(m_hwnd, &pos);
  SetCursorPos(pos.x, pos.y);
}

bool WinWindow::setNativeMouseCursor(NativeCursor cursor)
{
  HCURSOR hcursor = NULL;

  switch (cursor) {
    case kNoCursor:
      // Do nothing, just set to null
      break;
    case kArrowCursor:
      hcursor = LoadCursor(NULL, IDC_ARROW);
      break;
    case kCrosshairCursor:
      hcursor = LoadCursor(NULL, IDC_CROSS);
      break;
    case kIBeamCursor:
      hcursor = LoadCursor(NULL, IDC_IBEAM);
      break;
    case kWaitCursor:
      hcursor = LoadCursor(NULL, IDC_WAIT);
      break;
    case kLinkCursor:
      hcursor = LoadCursor(NULL, IDC_HAND);
      break;
    case kHelpCursor:
      hcursor = LoadCursor(NULL, IDC_HELP);
      break;
    case kForbiddenCursor:
      hcursor = LoadCursor(NULL, IDC_NO);
      break;
    case kMoveCursor:
      hcursor = LoadCursor(NULL, IDC_SIZEALL);
      break;
    case kSizeNCursor:
    case kSizeNSCursor:
    case kSizeSCursor:
      hcursor = LoadCursor(NULL, IDC_SIZENS);
      break;
    case kSizeECursor:
    case kSizeWCursor:
    case kSizeWECursor:
      hcursor = LoadCursor(NULL, IDC_SIZEWE);
      break;
    case kSizeNWCursor:
    case kSizeSECursor:
      hcursor = LoadCursor(NULL, IDC_SIZENWSE);
      break;
    case kSizeNECursor:
    case kSizeSWCursor:
      hcursor = LoadCursor(NULL, IDC_SIZENESW);
      break;
  }

  return setCursor(hcursor, false);
}

bool WinWindow::setNativeMouseCursor(const she::Surface* surface,
                                     const gfx::Point& focus,
                                     const int scale)
{
  ASSERT(surface);

  SurfaceFormatData format;
  surface->getFormat(&format);

  // Only for 32bpp surfaces
  if (format.bitsPerPixel != 32)
    return false;

  // Based on the following article "How To Create an Alpha
  // Blended Cursor or Icon in Windows XP":
  // https://support.microsoft.com/en-us/kb/318876

  int w = scale*surface->width();
  int h = scale*surface->height();

  BITMAPV5HEADER bi;
  ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
  bi.bV5Size = sizeof(BITMAPV5HEADER);
  bi.bV5Width = w;
  bi.bV5Height = h;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  bi.bV5RedMask = 0x00ff0000;
  bi.bV5GreenMask = 0x0000ff00;
  bi.bV5BlueMask = 0x000000ff;
  bi.bV5AlphaMask = 0xff000000;

  uint32_t* bits;
  HDC hdc = GetDC(nullptr);
  HBITMAP hbmp = CreateDIBSection(
    hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS,
    (void**)&bits, NULL, (DWORD)0);
  ReleaseDC(nullptr, hdc);
  if (!hbmp)
    return false;

  for (int y=0; y<h; ++y) {
    const uint32_t* ptr = (const uint32_t*)surface->getData(0, (h-1-y)/scale);
    for (int x=0, u=0; x<w; ++x, ++bits) {
      uint32_t c = *ptr;
      *bits =
        (((c & format.alphaMask) >> format.alphaShift) << 24) |
        (((c & format.redMask  ) >> format.redShift  ) << 16) |
        (((c & format.greenMask) >> format.greenShift) << 8) |
        (((c & format.blueMask ) >> format.blueShift ));
      if (++u == scale) {
        u = 0;
        ++ptr;
      }
    }
  }

  // Create an empty mask bitmap.
  HBITMAP hmonobmp = CreateBitmap(w, h, 1, 1, nullptr);
  if (!hmonobmp) {
    DeleteObject(hbmp);
    return false;
  }

  ICONINFO ii;
  ii.fIcon = FALSE;
  ii.xHotspot = scale*focus.x + scale/2;
  ii.yHotspot = scale*focus.y + scale/2;
  ii.hbmMask = hmonobmp;
  ii.hbmColor = hbmp;

  HCURSOR hcursor = CreateIconIndirect(&ii);

  DeleteObject(hbmp);
  DeleteObject(hmonobmp);

  return setCursor(hcursor, true);
}

void WinWindow::updateWindow(const gfx::Rect& bounds)
{
  RECT rc = { bounds.x*m_scale,
              bounds.y*m_scale,
              bounds.x*m_scale+bounds.w*m_scale,
              bounds.y*m_scale+bounds.h*m_scale };
  InvalidateRect(m_hwnd, &rc, FALSE);
  UpdateWindow(m_hwnd);
}

std::string WinWindow::getLayout()
{
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);
  if (GetWindowPlacement(m_hwnd, &wp)) {
    std::ostringstream s;
    s << 1 << ' '
      << wp.flags << ' '
      << wp.showCmd << ' '
      << wp.ptMinPosition.x << ' '
      << wp.ptMinPosition.y << ' '
      << wp.ptMaxPosition.x << ' '
      << wp.ptMaxPosition.y << ' '
      << wp.rcNormalPosition.left << ' '
      << wp.rcNormalPosition.top << ' '
      << wp.rcNormalPosition.right << ' '
      << wp.rcNormalPosition.bottom;
    return s.str();
  }
  return "";
}

void WinWindow::setLayout(const std::string& layout)
{
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);

  std::istringstream s(layout);
  int ver;
  s >> ver;
  if (ver == 1) {
    s >> wp.flags
      >> wp.showCmd
      >> wp.ptMinPosition.x
      >> wp.ptMinPosition.y
      >> wp.ptMaxPosition.x
      >> wp.ptMaxPosition.y
      >> wp.rcNormalPosition.left
      >> wp.rcNormalPosition.top
      >> wp.rcNormalPosition.right
      >> wp.rcNormalPosition.bottom;
  }
  else
    return;

  if (SetWindowPlacement(m_hwnd, &wp)) {
    // TODO use the return value
  }
}

void WinWindow::setTranslateDeadKeys(bool state)
{
  m_translateDeadKeys = state;

  // Here we clear dead keys so we don't get those keys in the new
  // "translate dead keys" state. E.g. If we focus a text entry
  // field and the translation of dead keys is enabled, we don't
  // want to get previous dead keys. The same in case we leave the
  // text field with a pending dead key, that dead key must be
  // discarded.
  VkToUnicode tu;
  if (tu) {
    tu.toUnicode(VK_SPACE, 0);
    if (tu.size() != 0)
      tu.toUnicode(VK_SPACE, 0);
  }
}

bool WinWindow::setCursor(HCURSOR hcursor, bool custom)
{
  SetCursor(hcursor);
  if (m_hcursor && m_customHcursor)
    DestroyIcon(m_hcursor);
  m_hcursor = hcursor;
  m_customHcursor = custom;
  return (hcursor ? true: false);
}

LRESULT WinWindow::wndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {

    case WM_CREATE:
      LOG("WIN: Creating window %p\n", m_hwnd);

      if (system()->useWintabAPI()) {
        // Attach Wacom context
        m_hpenctx = system()->penApi().open(m_hwnd);
      }
      break;

    case WM_DESTROY:
      LOG("WIN: Destroying window %p (pen context %p)\n",
          m_hwnd, m_hpenctx);

      if (m_hpenctx) {
        system()->penApi().close(m_hpenctx);
        m_hpenctx = nullptr;
      }
      break;

    case WM_SETCURSOR:
      if (LOWORD(lparam) == HTCLIENT) {
        SetCursor(m_hcursor);
        return TRUE;
      }
      break;

    case WM_CLOSE: {
      Event ev;
      ev.setType(Event::CloseDisplay);
      queueEvent(ev);

      // Don't close the window, it must be closed manually after
      // the CloseDisplay event is processed.
      return 0;
    }

    case WM_PAINT:
      if (m_isCreated) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        onPaint(hdc);
        EndPaint(m_hwnd, &ps);
        return true;
      }
      break;

    case WM_SIZE:
      if (m_isCreated) {
        int w = GET_X_LPARAM(lparam);
        int h = GET_Y_LPARAM(lparam);

        if (w > 0 && h > 0) {
          m_clientSize.w = w;
          m_clientSize.h = h;
          onResize(m_clientSize);
        }

        WINDOWPLACEMENT pl;
        pl.length = sizeof(pl);
        if (GetWindowPlacement(m_hwnd, &pl)) {
          m_restoredSize = gfx::Size(
            pl.rcNormalPosition.right - pl.rcNormalPosition.left,
            pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
        }
      }
      break;

    // Mouse and Trackpad Messages

    case WM_MOUSEMOVE: {
      // If the pointer API is enable, we use WM_POINTERUPDATE instead
      // of WM_MOUSEMOVE.  This check is here because Windows keeps
      // sending us WM_MOUSEMOVE messages even when we call
      // EnableMouseInPointer() (mainly when we use Alt+stylus we
      // receive WM_MOUSEMOVE with the position of the mouse/trackpad
      // + WM_POINTERUPDATE with the position of the pen)
      if (m_ignoreMouseMessages)
        break;

      Event ev;
      mouseEvent(lparam, ev);

      MOUSE_TRACE("MOUSEMOVE xy=%d,%d\n",
                  ev.position().x, ev.position().y);

      if (!m_hasMouse) {
        m_hasMouse = true;

        ev.setType(Event::MouseEnter);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseEnter\n");

        // Track mouse to receive WM_MOUSELEAVE message.
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        _TrackMouseEvent(&tme);
      }

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }

      ev.setType(Event::MouseMove);
      queueEvent(ev);
      break;
    }

    case WM_NCMOUSEMOVE:
    case WM_MOUSELEAVE:
      if (m_hasMouse) {
        m_hasMouse = false;

        Event ev;
        ev.setType(Event::MouseLeave);
        ev.setModifiers(get_modifiers_from_last_win32_message());
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseLeave\n");
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseDown);
      ev.setButton(
        msg == WM_LBUTTONDOWN ? Event::LeftButton:
        msg == WM_RBUTTONDOWN ? Event::RightButton:
        msg == WM_MBUTTONDOWN ? Event::MiddleButton:
        msg == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button:
        msg == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button:
        Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }
      queueEvent(ev);

      MOUSE_TRACE("BUTTONDOWN xy=%d,%d button=%d\n",
                  ev.position().x, ev.position().y,
                  ev.button());
      break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseUp);
      ev.setButton(
        msg == WM_LBUTTONUP ? Event::LeftButton:
        msg == WM_RBUTTONUP ? Event::RightButton:
        msg == WM_MBUTTONUP ? Event::MiddleButton:
        msg == WM_XBUTTONUP && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button:
        msg == WM_XBUTTONUP && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button:
        Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }
      queueEvent(ev);

      MOUSE_TRACE("BUTTONUP xy=%d,%d button=%d\n",
                  ev.position().x, ev.position().y,
                  ev.button());

      // Avoid popup menu for scrollbars
      if (msg == WM_RBUTTONUP)
        return 0;

      break;
    }

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK: {
      Event ev;
      mouseEvent(lparam, ev);
      ev.setType(Event::MouseDoubleClick);
      ev.setButton(
        msg == WM_LBUTTONDBLCLK ? Event::LeftButton:
        msg == WM_RBUTTONDBLCLK ? Event::RightButton:
        msg == WM_MBUTTONDBLCLK ? Event::MiddleButton:
        msg == WM_XBUTTONDBLCLK && GET_XBUTTON_WPARAM(wparam) == 1 ? Event::X1Button:
        msg == WM_XBUTTONDBLCLK && GET_XBUTTON_WPARAM(wparam) == 2 ? Event::X2Button:
        Event::NoneButton);

      if (m_pointerType != PointerType::Unknown) {
        ev.setPointerType(m_pointerType);
        ev.setPressure(m_pressure);
      }
      queueEvent(ev);

      MOUSE_TRACE("BUTTONDBLCLK xy=%d,%d button=%d\n",
                  ev.position().x, ev.position().y,
                  ev.button());
      break;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      POINT pos = { GET_X_LPARAM(lparam),
                    GET_Y_LPARAM(lparam) };
      ScreenToClient(m_hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setPosition(gfx::Point(pos.x, pos.y) / m_scale);

      int z = GET_WHEEL_DELTA_WPARAM(wparam);
      if (ABS(z) >= WHEEL_DELTA)
        z /= WHEEL_DELTA;
      else {
        // TODO use floating point numbers or something similar
        //      (so we could use: z /= double(WHEEL_DELTA))
        z = SGN(z);
      }

      gfx::Point delta(
        (msg == WM_MOUSEHWHEEL ? z: 0),
        (msg == WM_MOUSEWHEEL ? -z: 0));
      ev.setWheelDelta(delta);
      queueEvent(ev);

      MOUSE_TRACE("MOUSEWHEEL xy=%d,%d delta=%d,%d\n",
                  ev.position().x, ev.position().y,
                  ev.wheelDelta().x, ev.wheelDelta().y);
      break;
    }

    case WM_HSCROLL:
    case WM_VSCROLL: {
      POINT pos;
      GetCursorPos(&pos);
      ScreenToClient(m_hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setPosition(gfx::Point(pos.x, pos.y) / m_scale);

      int bar = (msg == WM_HSCROLL ? SB_HORZ: SB_VERT);
      int z = GetScrollPos(m_hwnd, bar);

      switch (LOWORD(wparam)) {
        case SB_LEFT:
        case SB_LINELEFT:
          --z;
          break;
        case SB_PAGELEFT:
          z -= 2;
          break;
        case SB_RIGHT:
        case SB_LINERIGHT:
          ++z;
          break;
        case SB_PAGERIGHT:
          z += 2;
          break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
          // Do nothing
          break;
      }

      gfx::Point delta(
        (msg == WM_HSCROLL ? (z-50): 0),
        (msg == WM_VSCROLL ? (z-50): 0));
      ev.setWheelDelta(delta);

      SetScrollPos(m_hwnd, bar, 50, FALSE);
      queueEvent(ev);

      MOUSE_TRACE("HVSCROLL xy=%d,%d delta=%d,%d\n",
                  ev.position().x, ev.position().y,
                  ev.wheelDelta().x, ev.wheelDelta().y);
      break;
    }

    // Pointer API (since Windows 8.0)

    case WM_POINTERCAPTURECHANGED: {
      MOUSE_TRACE("POINTERCAPTURECHANGED\n");
      releaseMouse();
      break;
    }

    case WM_POINTERENTER: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      MOUSE_TRACE("POINTERENTER id=%d xy=%d,%d\n",
                  pi.pointerId, ev.position().x, ev.position().y);

#if USE_EnableMouseInPointer == 0
      // This is necessary to avoid receiving random WM_MOUSEMOVE from
      // the mouse position when we use Alt+pen tip.
      // TODO Remove this line when we enable EnableMouseInPointer(TRUE);
      m_ignoreMouseMessages = true;
#endif

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.AddPointerInteractionContext)
          winApi.AddPointerInteractionContext(m_ictx, pi.pointerId);
      }

      if (!m_hasMouse) {
        m_hasMouse = true;

        ev.setType(Event::MouseEnter);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseEnter\n");
      }
      return 0;
    }

    case WM_POINTERLEAVE: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      MOUSE_TRACE("POINTERLEAVE id=%d\n", pi.pointerId);

#if USE_EnableMouseInPointer == 0
      m_ignoreMouseMessages = false;
#endif

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.RemovePointerInteractionContext)
          winApi.RemovePointerInteractionContext(m_ictx, pi.pointerId);
      }

#if 0 // Don't generate MouseLeave from pen/touch messages
      // TODO we should generate this message, but after this touch
      //      messages don't work anymore, so we have to fix that problem.
      if (m_hasMouse) {
        m_hasMouse = false;

        ev.setType(Event::MouseLeave);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseLeave\n");
        return 0;
      }
#endif
      break;
    }

    case WM_POINTERDOWN: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      ev.setType(Event::MouseDown);
      ev.setButton(
        pi.ButtonChangeType == POINTER_CHANGE_FIRSTBUTTON_DOWN ? Event::LeftButton:
        pi.ButtonChangeType == POINTER_CHANGE_SECONDBUTTON_DOWN ? Event::RightButton:
        pi.ButtonChangeType == POINTER_CHANGE_THIRDBUTTON_DOWN ? Event::MiddleButton:
        pi.ButtonChangeType == POINTER_CHANGE_FOURTHBUTTON_DOWN ? Event::X1Button:
        pi.ButtonChangeType == POINTER_CHANGE_FIFTHBUTTON_DOWN ? Event::X2Button:
        Event::NoneButton);
      queueEvent(ev);

      MOUSE_TRACE("POINTERDOWN id=%d xy=%d,%d button=%d\n",
                  pi.pointerId, ev.position().x, ev.position().y,
                  ev.button());
      return 0;
    }

    case WM_POINTERUP: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      ev.setType(Event::MouseUp);
      ev.setButton(
        pi.ButtonChangeType == POINTER_CHANGE_FIRSTBUTTON_UP ? Event::LeftButton:
        pi.ButtonChangeType == POINTER_CHANGE_SECONDBUTTON_UP ? Event::RightButton:
        pi.ButtonChangeType == POINTER_CHANGE_THIRDBUTTON_UP ? Event::MiddleButton:
        pi.ButtonChangeType == POINTER_CHANGE_FOURTHBUTTON_UP ? Event::X1Button:
        pi.ButtonChangeType == POINTER_CHANGE_FIFTHBUTTON_UP ? Event::X2Button:
        Event::NoneButton);
      queueEvent(ev);

      MOUSE_TRACE("POINTERUP id=%d xy=%d,%d button=%d\n",
                  pi.pointerId, ev.position().x, ev.position().y,
                  ev.button());
      return 0;
    }

    case WM_POINTERUPDATE: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      if (!m_hasMouse) {
        m_hasMouse = true;

        ev.setType(Event::MouseEnter);
        queueEvent(ev);

        MOUSE_TRACE("-> Event::MouseEnter\n");
      }

      ev.setType(Event::MouseMove);
      queueEvent(ev);

      MOUSE_TRACE("POINTERUPDATE id=%d xy=%d,%d\n",
                  pi.pointerId, ev.position().x, ev.position().y);
      return 0;
    }

    case WM_POINTERWHEEL:
    case WM_POINTERHWHEEL: {
      POINTER_INFO pi;
      Event ev;
      if (!pointerEvent(wparam, ev, pi))
        break;

      ev.setType(Event::MouseWheel);

      int z = GET_WHEEL_DELTA_WPARAM(wparam);
      if (ABS(z) >= WHEEL_DELTA)
        z /= WHEEL_DELTA;
      else {
        // TODO use floating point numbers or something similar
        //      (so we could use: z /= double(WHEEL_DELTA))
        z = SGN(z);
      }

      gfx::Point delta(
        (msg == WM_POINTERHWHEEL ? z: 0),
        (msg == WM_POINTERWHEEL ? -z: 0));
      ev.setWheelDelta(delta);
      queueEvent(ev);

      MOUSE_TRACE("POINTERWHEEL xy=%d,%d delta=%d,%d\n",
                  ev.position().x, ev.position().y,
                  ev.wheelDelta().x, ev.wheelDelta().y);

      return 0;
    }

    // Keyboard Messages

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
      int vk = wparam;
      int scancode = (lparam >> 16) & 0xff;
      bool sendMsg = true;

      Event ev;
      ev.setType(Event::KeyDown);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setScancode(win32vk_to_scancode(vk));
      ev.setUnicodeChar(0);
      ev.setRepeat(MAX(0, (lparam & 0xffff)-1));

      KEY_TRACE("KEYDOWN vk=%d scancode=%d->%d modifiers=%d\n",
                vk, scancode, ev.scancode(), ev.modifiers());

      {
        VkToUnicode tu;
        if (tu) {
          tu.toUnicode(vk, scancode);
          if (tu.isDeadKey()) {
            ev.setDeadKey(true);
            ev.setUnicodeChar(tu[0]);
            if (!m_translateDeadKeys)
              tu.toUnicode(vk, scancode); // Call again to remove dead-key
          }
          else if (tu.size() > 0) {
            sendMsg = false;
            for (int chr : tu) {
              ev.setUnicodeChar(chr);
              queueEvent(ev);

              KEY_TRACE(" -> queued unicode char=%d <%c>\n",
                        ev.unicodeChar(),
                        ev.unicodeChar() ? ev.unicodeChar(): ' ');
            }
          }
        }
      }

      if (sendMsg) {
        queueEvent(ev);
        KEY_TRACE(" -> queued unicode char=%d <%c>\n",
                  ev.unicodeChar(),
                  ev.unicodeChar() ? ev.unicodeChar(): ' ');
      }

      return 0;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP: {
      Event ev;
      ev.setType(Event::KeyUp);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setScancode(win32vk_to_scancode(wparam));
      ev.setUnicodeChar(0);
      ev.setRepeat(MAX(0, (lparam & 0xffff)-1));
      queueEvent(ev);

      // TODO If we use native menus, this message should be given
      // to the DefWindowProc() in some cases (e.g. F10 or Alt keys)
      return 0;
    }

    case WM_MENUCHAR:
      // Avoid playing a sound when Alt+key is pressed and it's not in a native menu
      return MAKELONG(0, MNC_CLOSE);

    case WM_DROPFILES: {
      HDROP hdrop = (HDROP)(wparam);
      Event::Files files;

      int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
      for (int index=0; index<count; ++index) {
        int length = DragQueryFile(hdrop, index, NULL, 0);
        if (length > 0) {
          std::vector<TCHAR> str(length+1);
          DragQueryFile(hdrop, index, &str[0], str.size());
          files.push_back(base::to_utf8(&str[0]));
        }
      }

      DragFinish(hdrop);

      Event ev;
      ev.setType(Event::DropFiles);
      ev.setFiles(files);
      queueEvent(ev);
      break;
    }

    case WM_NCCALCSIZE: {
      if (wparam) {
        // Scrollbars must be enabled and visible to get trackpad
        // events of old drivers. So we cannot use ShowScrollBar() to
        // hide them. This is a simple (maybe not so elegant)
        // solution: Expand the client area to we overlap the
        // scrollbars. In this way they are not visible, but we still
        // get their messages.
        NCCALCSIZE_PARAMS* cs = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
        cs->rgrc[0].right += GetSystemMetrics(SM_CYVSCROLL);
        cs->rgrc[0].bottom += GetSystemMetrics(SM_CYHSCROLL);
      }
      break;
    }

    case WM_NCHITTEST: {
      LRESULT result = CallWindowProc(DefWindowProc, m_hwnd, msg, wparam, lparam);
      gfx::Point pt(GET_X_LPARAM(lparam),
                    GET_Y_LPARAM(lparam));

      ABS_CLIENT_RC(rc);
      gfx::Rect area(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

      //LOG("NCHITTEST: %d %d - %d %d %d %d - %s\n", pt.x, pt.y, area.x, area.y, area.w, area.h, area.contains(pt) ? "true": "false");

      // We ignore scrollbars so if the mouse is above them, we return
      // as it's in the window client or resize area. (Remember that
      // we have scroll bars are enabled and visible to receive
      // trackpad messages only.)
      if (result == HTHSCROLL) {
        result = (area.contains(pt) ? HTCLIENT: HTBOTTOM);
      }
      else if (result == HTVSCROLL) {
        result = (area.contains(pt) ? HTCLIENT: HTRIGHT);
      }

      return result;
    }

    // Wintab API Messages

    case WT_PROXIMITY: {
      bool entering_ctx = (LOWORD(lparam) ? true: false);
      if (!entering_ctx)
        m_pointerType = PointerType::Unknown;
      break;
    }

    case WT_CSRCHANGE: {    // From Wintab 1.1
      auto& api = system()->penApi();
      UINT serial = wparam;
      HCTX ctx = (HCTX)lparam;
      PACKET packet;

      if (api.packet(ctx, serial, &packet)) {
        switch (packet.pkCursor) {
          case 0:
          case 3:
            m_pointerType = PointerType::Cursor;
            break;
          case 1:
          case 4:
            m_pointerType = PointerType::Pen;
            break;
          case 2:
          case 5:
            m_pointerType = PointerType::Eraser;
            break;
          default:
            m_pointerType = PointerType::Unknown;
            break;
        }
      }
      else
        m_pointerType = PointerType::Unknown;
    }

    case WT_PACKET: {
      auto& api = system()->penApi();
      UINT serial = wparam;
      HCTX ctx = (HCTX)lparam;
      PACKET packet;

      if (api.packet(ctx, serial, &packet)) {
        m_pressure = packet.pkNormalPressure / 1000.0; // TODO get the maximum value

        if (packet.pkCursor == 2 || packet.pkCursor == 5)
          m_pointerType = PointerType::Eraser;
        else
          m_pointerType = PointerType::Pen;
      }
      else
        m_pointerType = PointerType::Unknown;
      break;
    }

  }

  LRESULT result = FALSE;
  if (handle_dde_messages(m_hwnd, msg, wparam, lparam, result))
    return result;

  return DefWindowProc(m_hwnd, msg, wparam, lparam);
}

void WinWindow::mouseEvent(LPARAM lparam, Event& ev)
{
  ev.setModifiers(get_modifiers_from_last_win32_message());
  ev.setPosition(gfx::Point(
                   GET_X_LPARAM(lparam) / m_scale,
                   GET_Y_LPARAM(lparam) / m_scale));
}

bool WinWindow::pointerEvent(WPARAM wparam, Event& ev, POINTER_INFO& pi)
{
  if (!m_usePointerApi)
    return false;

  auto& winApi = system()->winApi();
  if (!winApi.GetPointerInfo(GET_POINTERID_WPARAM(wparam), &pi))
    return false;

  ABS_CLIENT_RC(rc);

  ev.setModifiers(get_modifiers_from_last_win32_message());
  ev.setPosition(gfx::Point((pi.ptPixelLocation.x - rc.left) / m_scale,
                            (pi.ptPixelLocation.y - rc.top) / m_scale));

  switch (pi.pointerType) {
    case PT_MOUSE: {
      ev.setPointerType(PointerType::Mouse);
      break;
    }
    case PT_TOUCH: {
      ev.setPointerType(PointerType::Touch);
      break;
    }
    case PT_TOUCHPAD: {
      ev.setPointerType(PointerType::Touchpad);
      break;
    }
    case PT_PEN: {
      ev.setPointerType(PointerType::Pen);

      POINTER_PEN_INFO ppi;
      if (winApi.GetPointerPenInfo(pi.pointerId, &ppi)) {
        if (ppi.penFlags & PEN_FLAG_ERASER)
          ev.setPointerType(PointerType::Eraser);
      }
      break;
    }
  }

  m_lastPointerId = pi.pointerId;
  return true;
}

void WinWindow::handleInteractionContextOutput(
  const INTERACTION_CONTEXT_OUTPUT* output)
{
  MOUSE_TRACE("%s (%d) xy=%.16g %.16g flags=%d type=%d\n",
              output->interactionId == INTERACTION_ID_MANIPULATION ? "INTERACTION_ID_MANIPULATION":
              output->interactionId == INTERACTION_ID_TAP ? "INTERACTION_ID_TAP":
              output->interactionId == INTERACTION_ID_SECONDARY_TAP ? "INTERACTION_ID_SECONDARY_TAP":
              output->interactionId == INTERACTION_ID_HOLD ? "INTERACTION_ID_HOLD": "INTERACTION_ID_???",
              output->interactionId,
              output->x, output->y,
              output->interactionFlags,
              output->inputType);

  // We use the InteractionContext to interpret touch gestures only and double tap with pen.
  if ((output->inputType == PT_TOUCH) ||
      (output->inputType == PT_PEN &&
       output->interactionId == INTERACTION_ID_TAP &&
       output->arguments.tap.count == 2)) {
    ABS_CLIENT_RC(rc);

    gfx::Point pos(int((output->x - rc.left) / m_scale),
                   int((output->y - rc.top) / m_scale));

    Event ev;
    ev.setModifiers(get_modifiers_from_last_win32_message());
    ev.setPosition(pos);

    bool hadMouse = m_hasMouse;
    if (!m_hasMouse) {
      m_hasMouse = true;
      ev.setType(Event::MouseEnter);
      queueEvent(ev);
    }

    switch (output->interactionId) {
      case INTERACTION_ID_MANIPULATION: {
        MOUSE_TRACE(" - delta xy=%.16g %.16g scale=%.16g expansion=%.16g rotation=%.16g\n",
                    output->arguments.manipulation.delta.translationX,
                    output->arguments.manipulation.delta.translationY,
                    output->arguments.manipulation.delta.scale,
                    output->arguments.manipulation.delta.expansion,
                    output->arguments.manipulation.delta.rotation);

        gfx::Point delta(-int(output->arguments.manipulation.delta.translationX) / m_scale,
                         -int(output->arguments.manipulation.delta.translationY) / m_scale);

        if (output->interactionFlags & INTERACTION_FLAG_BEGIN) {
          ev.setType(Event::MouseMove);
          queueEvent(ev);
        }

        ev.setType(Event::MouseWheel);
        ev.setWheelDelta(delta);
        ev.setPreciseWheel(true);
        queueEvent(ev);

        ev.setType(Event::TouchMagnify);
        ev.setMagnification(output->arguments.manipulation.delta.scale - 1.0);
        queueEvent(ev);
        break;
      }

      case INTERACTION_ID_TAP:
        MOUSE_TRACE(" - count=%d\n", output->arguments.tap.count);

        ev.setButton(Event::LeftButton);
        ev.setType(Event::MouseMove); queueEvent(ev);
        if (output->arguments.tap.count == 2) {
          ev.setType(Event::MouseDoubleClick); queueEvent(ev);
        }
        else {
          ev.setType(Event::MouseDown); queueEvent(ev);
          ev.setType(Event::MouseUp); queueEvent(ev);
        }
        break;

      case INTERACTION_ID_SECONDARY_TAP:
      case INTERACTION_ID_HOLD:
        ev.setButton(Event::RightButton);
        ev.setType(Event::MouseMove); queueEvent(ev);
        ev.setType(Event::MouseDown); queueEvent(ev);
        ev.setType(Event::MouseUp); queueEvent(ev);
        break;
    }
  }
}

//static
void WinWindow::registerClass()
{
  HMODULE instance = GetModuleHandle(nullptr);

  WNDCLASSEX wcex;
  if (GetClassInfoEx(instance, SHE_WND_CLASS_NAME, &wcex))
    return;                 // Already registered

  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_DBLCLKS;
  wcex.lpfnWndProc   = &WinWindow::staticWndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = 0;
  wcex.hInstance     = instance;
  wcex.hIcon         = LoadIcon(instance, L"0");
  wcex.hCursor       = NULL;
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName  = nullptr;
  wcex.lpszClassName = SHE_WND_CLASS_NAME;
  wcex.hIconSm       = nullptr;

  if (RegisterClassEx(&wcex) == 0)
    throw std::runtime_error("Error registering window class");
}

//static
HWND WinWindow::createHwnd(WinWindow* self, int width, int height)
{
  HWND hwnd = CreateWindowEx(
    WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
    SHE_WND_CLASS_NAME,
    L"",
    WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    nullptr,
    nullptr,
    GetModuleHandle(nullptr),
    reinterpret_cast<LPVOID>(self));
  if (!hwnd)
    return nullptr;

  // Center the window
  RECT workarea;
  if (SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&workarea, 0)) {
    SetWindowPos(hwnd, nullptr,
                 (workarea.right-workarea.left)/2-width/2,
                 (workarea.bottom-workarea.top)/2-height/2, 0, 0,
                 SWP_NOSIZE |
                 SWP_NOSENDCHANGING |
                 SWP_NOOWNERZORDER |
                 SWP_NOZORDER |
                 SWP_NOREDRAW);
  }

  // Set scroll info to receive WM_HSCROLL/VSCROLL events (events
  // generated by some trackpad drivers).
  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
  si.nMin = 0;
  si.nPos = 50;
  si.nMax = 100;
  si.nPage = 10;
  SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);
  SetScrollInfo(hwnd, SB_VERT, &si, FALSE);

  return hwnd;
}

//static
LRESULT CALLBACK WinWindow::staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  WinWindow* wnd = nullptr;

  if (msg == WM_CREATE) {
    wnd =
      reinterpret_cast<WinWindow*>(
        reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);

    if (wnd && wnd->m_hwnd == nullptr)
      wnd->m_hwnd = hwnd;
  }
  else {
    wnd = reinterpret_cast<WinWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // Check that the user data makes sense
    if (wnd && wnd->m_hwnd != hwnd)
      wnd = nullptr;
  }

  if (wnd) {
    ASSERT(wnd->m_hwnd == hwnd);
    return wnd->wndProc(msg, wparam, lparam);
  }
  else {
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

//static
void CALLBACK WinWindow::staticInteractionContextCallback(
  void* clientData,
  const INTERACTION_CONTEXT_OUTPUT* output)
{
  WinWindow* self = reinterpret_cast<WinWindow*>(clientData);
  self->handleInteractionContextOutput(output);
}

// static
WindowSystem* WinWindow::system()
{
  return static_cast<WindowSystem*>(she::instance());
}

} // namespace she
