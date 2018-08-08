// SHE library
// Copyright (C) 2012-2018  David Capello
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
#include "she/win/keys.h"
#include "she/win/system.h"
#include "she/win/window_dde.h"

#define SHE_WND_CLASS_NAME L"Aseprite.Window"

#define KEY_TRACE(...)
#define MOUSE_TRACE(...)
#define TOUCH_TRACE(...)

#define kFingerAsMouseTimeout 50

// Gets the window client are in absolute/screen coordinates
#define ABS_CLIENT_RC(rc)                               \
  RECT rc;                                              \
  GetClientRect(m_hwnd, &rc);                           \
  MapWindowPoints(m_hwnd, NULL, (POINT*)&rc, 2)

#ifndef INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN
#define INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS_SCREEN 1
#endif

namespace she {

static PointerType wt_packet_pkcursor_to_pointer_type(int pkCursor)
{
  switch (pkCursor) {
    case 0:
    case 3:
      return PointerType::Cursor;
    case 1:
    case 4:
      return PointerType::Pen;
    case 2:
    case 5:
    case 6: // Undocumented: Inverted stylus when EnableMouseInPointer() is on
      return PointerType::Eraser;
  }
  return PointerType::Unknown;
}

WinWindow::Touch::Touch()
  : fingers(0)
  , canBeMouse(false)
  , asMouse(false)
  , timerID(0)
{
}

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
  , m_lastPointerId(0)
  , m_ictx(nullptr)
  , m_ignoreRandomMouseEvents(0)
  // True by default, we prefer to interpret one finger as mouse movement
  , m_touch(new Touch)
#if SHE_USE_POINTER_API_FOR_MOUSE
  , m_emulateDoubleClick(false)
  , m_doubleClickMsecs(GetDoubleClickTime())
  , m_lastPointerDownTime(0)
  , m_lastPointerDownButton(Event::NoneButton)
  , m_pointerDownCount(0)
#endif
  , m_hpenctx(nullptr)
  , m_pointerType(PointerType::Unknown)
  , m_pressure(0.0)
{
  auto& winApi = system()->winApi();
  if (winApi.EnableMouseInPointer &&
      winApi.IsMouseInPointerEnabled &&
      winApi.GetPointerInfo &&
      winApi.GetPointerPenInfo) {
    // Do not enable pointer API for mouse events because:
    // - Wacom driver doesn't inform their messages in a correct
    //   pointer API format (events from pen are reported as mouse
    //   events and without eraser tip information).
    // - We have to emulate the double-click for the regular mouse
    //   (search for m_emulateDoubleClick).
    // - Double click with Wacom stylus doesn't work.
#if SHE_USE_POINTER_API_FOR_MOUSE
    if (!winApi.IsMouseInPointerEnabled()) {
      // Prefer pointer messages (WM_POINTER*) since Windows 8 instead
      // of mouse messages (WM_MOUSE*)
      winApi.EnableMouseInPointer(TRUE);
      m_emulateDoubleClick =
        (winApi.IsMouseInPointerEnabled() ? true: false);
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

void WinWindow::setInterpretOneFingerGestureAsMouseMovement(bool state)
{
  if (state) {
    if (!m_touch)
      m_touch = new Touch;
  }
  else if (m_touch) {
    killTouchTimer();
    delete m_touch;
    m_touch = nullptr;
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
      Event ev;
      mouseEvent(lparam, ev);

      MOUSE_TRACE("MOUSEMOVE xy=%d,%d\n",
                  ev.position().x, ev.position().y);

      if (m_ignoreRandomMouseEvents > 0) {
        MOUSE_TRACE(" - IGNORED\n");
        --m_ignoreRandomMouseEvents;
        break;
      }

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

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.AddPointerInteractionContext) {
          winApi.AddPointerInteractionContext(m_ictx, pi.pointerId);

          if (m_touch && pi.pointerType == PT_TOUCH &&
              !m_touch->asMouse) {
            ++m_touch->fingers;
            TOUCH_TRACE("POINTERENTER fingers=%d\n", m_touch->fingers);
            if (m_touch->fingers == 1) {
              waitTimerToConvertFingerAsMouseMovement();
            }
            else if (m_touch->canBeMouse && m_touch->fingers >= 2) {
              delegateFingerToInteractionContext();
            }
          }
        }
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

      // After releasing a finger a WM_MOUSEMOVE event in the trackpad
      // position is generated, we'll ignore that message.
      if (m_touch)
        m_ignoreRandomMouseEvents = 1;
      else
        m_ignoreRandomMouseEvents = 0;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.RemovePointerInteractionContext) {
          winApi.RemovePointerInteractionContext(m_ictx, pi.pointerId);

          if (m_touch && pi.pointerType == PT_TOUCH) {
            if (m_touch->fingers > 0)
              --m_touch->fingers;
            TOUCH_TRACE("POINTERLEAVE fingers=%d\n", m_touch->fingers);
            if (m_touch->fingers == 0) {
              if (m_touch->canBeMouse)
                sendDelayedTouchEvents();
              else
                clearDelayedTouchEvents();
              killTouchTimer();
              m_touch->asMouse = false;
            }
          }
        }
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
          if (!m_touch && pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      handlePointerButtonChange(ev, pi);

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
          if (!m_touch && pi.pointerType == PT_TOUCH)
            return 0;
        }
      }

      handlePointerButtonChange(ev, pi);

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

      // See the comment for m_ignoreRandomMouseEvents variable, and
      // why here is = 2.
      m_ignoreRandomMouseEvents = 2;

      if (pi.pointerType == PT_TOUCH || pi.pointerType == PT_PEN) {
        auto& winApi = system()->winApi();
        if (m_ictx && winApi.ProcessPointerFramesInteractionContext) {
          winApi.ProcessPointerFramesInteractionContext(m_ictx, 1, 1, &pi);
          if (!m_touch && pi.pointerType == PT_TOUCH)
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

      if (m_touch && pi.pointerType == PT_TOUCH) {
        TOUCH_TRACE("POINTERUPDATE canBeMouse=%d asMouse=%d\n",
                    m_touch->canBeMouse,
                    m_touch->asMouse);
        if (!m_touch->asMouse) {
          if (m_touch->canBeMouse)
            m_touch->delayedEvents.push_back(ev);
          else
            return 0;
        }
        else
          queueEvent(ev);
      }
      else
        queueEvent(ev);

      handlePointerButtonChange(ev, pi);

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

    case WM_TIMER:
      TOUCH_TRACE("TIMER %d\n", wparam);
      if (m_touch && m_touch->timerID == wparam) {
        killTouchTimer();

        if (!m_touch->asMouse &&
            m_touch->canBeMouse &&
            m_touch->fingers == 1) {
          TOUCH_TRACE("-> finger as mouse, sent %d events\n",
                      m_touch->delayedEvents.size());

          convertFingerAsMouseMovement();
        }
        else {
          delegateFingerToInteractionContext();
        }
      }
      break;

    // Keyboard Messages

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
      int vk = wparam;
      int scancode = (lparam >> 16) & 0xff;
      bool sendMsg = true;
      const KeyScancode sheScancode = win32vk_to_scancode(vk);

      // We only create one KeyDown event for modifiers. Bit 30
      // indicates the previous state of the key, if the modifier was
      // already pressed don't generate the event.
      if ((sheScancode >= kKeyFirstModifierScancode) &&
          (lparam & (1 << 30)))
        return 0;

      Event ev;
      ev.setType(Event::KeyDown);
      ev.setModifiers(get_modifiers_from_last_win32_message());
      ev.setScancode(sheScancode);
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
      base::paths files;

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
      MOUSE_TRACE("WT_PROXIMITY\n");

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

      if (api.packet(ctx, serial, &packet))
        m_pointerType = wt_packet_pkcursor_to_pointer_type(packet.pkCursor);
      else
        m_pointerType = PointerType::Unknown;

      MOUSE_TRACE("WT_CSRCHANGE pointer=%d\n", m_pointerType);
      break;
    }

    case WT_PACKET: {
      auto& api = system()->penApi();
      UINT serial = wparam;
      HCTX ctx = (HCTX)lparam;
      PACKET packet;

      if (api.packet(ctx, serial, &packet)) {
        m_pressure = packet.pkNormalPressure / 1000.0; // TODO get the maximum value
        m_pointerType = wt_packet_pkcursor_to_pointer_type(packet.pkCursor);
      }
      else
        m_pointerType = PointerType::Unknown;

      MOUSE_TRACE("WT_PACKET pointer=%d m_pressure=%.16g\n",
                  m_pointerType, m_pressure);
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
      MOUSE_TRACE("pi.pointerType PT_MOUSE\n");
      ev.setPointerType(PointerType::Mouse);

      // If we use EnableMouseInPointer(true), events from Wacom
      // stylus came as PT_MOUSE instead of PT_PEN with eraser
      // flag. This is just insane, EnableMouseInPointer(true) is not
      // an option at the moment if we want proper support for Wacom
      // events.
      break;
    }
    case PT_TOUCH: {
      MOUSE_TRACE("pi.pointerType PT_TOUCH\n");
      ev.setPointerType(PointerType::Touch);
      break;
    }
    case PT_TOUCHPAD: {
      MOUSE_TRACE("pi.pointerType PT_TOUCHPAD\n");
      ev.setPointerType(PointerType::Touchpad);
      break;
    }
    case PT_PEN: {
      MOUSE_TRACE("pi.pointerType PT_PEN\n");
      ev.setPointerType(PointerType::Pen);

      POINTER_PEN_INFO ppi;
      if (winApi.GetPointerPenInfo(pi.pointerId, &ppi)) {
        MOUSE_TRACE(" - ppi.penFlags = %d\n", ppi.penFlags);
        if (ppi.penFlags & PEN_FLAG_ERASER)
          ev.setPointerType(PointerType::Eraser);
      }
      break;
    }
  }

  m_lastPointerId = pi.pointerId;
  return true;
}

void WinWindow::handlePointerButtonChange(Event& ev, POINTER_INFO& pi)
{
  if (pi.ButtonChangeType == POINTER_CHANGE_NONE) {
#if SHE_USE_POINTER_API_FOR_MOUSE
    // Reset the counter of pointer down for the emulated double-click
    if (m_emulateDoubleClick)
      m_pointerDownCount = 0;
#endif
    return;
  }

  Event::MouseButton button = Event::NoneButton;
  bool down = false;

  switch (pi.ButtonChangeType) {
    case POINTER_CHANGE_FIRSTBUTTON_DOWN:
      down = true;
    case POINTER_CHANGE_FIRSTBUTTON_UP:
      button = Event::LeftButton;
      break;
    case  POINTER_CHANGE_SECONDBUTTON_DOWN:
      down = true;
    case POINTER_CHANGE_SECONDBUTTON_UP:
      button = Event::RightButton;
      break;
    case POINTER_CHANGE_THIRDBUTTON_DOWN:
      down = true;
    case POINTER_CHANGE_THIRDBUTTON_UP:
      button = Event::MiddleButton;
      break;
    case POINTER_CHANGE_FOURTHBUTTON_DOWN:
      down = true;
    case POINTER_CHANGE_FOURTHBUTTON_UP:
      button = Event::X1Button;
      break;
    case POINTER_CHANGE_FIFTHBUTTON_DOWN:
      down = true;
    case POINTER_CHANGE_FIFTHBUTTON_UP:
      button = Event::X2Button;
      break;
  }

  if (button == Event::NoneButton)
    return;

  ev.setType(down ? Event::MouseDown: Event::MouseUp);
  ev.setButton(button);

#if SHE_USE_POINTER_API_FOR_MOUSE
  if (down && m_emulateDoubleClick) {
    if (button != m_lastPointerDownButton)
      m_pointerDownCount = 0;

    ++m_pointerDownCount;

    base::tick_t curTime = base::current_tick();
    if ((m_pointerDownCount == 2) &&
        (curTime - m_lastPointerDownTime) <= m_doubleClickMsecs) {
      ev.setType(Event::MouseDoubleClick);
      m_pointerDownCount = 0;
    }

    m_lastPointerDownTime = curTime;
    m_lastPointerDownButton = button;
  }
#endif

  if (m_touch && pi.pointerType == PT_TOUCH) {
    if (!m_touch->asMouse) {
      if (m_touch->canBeMouse) {
        // TODO Review why the ui layer needs a Event::MouseMove event
        //      before ButtonDown/Up events.
        Event evMouseMove = ev;
        evMouseMove.setType(Event::MouseMove);
        m_touch->delayedEvents.push_back(evMouseMove);
        m_touch->delayedEvents.push_back(ev);
      }
      return;
    }
  }

  queueEvent(ev);
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

  // We use the InteractionContext to interpret touch gestures only
  // and double tap with pen.
  if ((output->inputType == PT_TOUCH
       && (!m_touch
           || (!m_touch->asMouse && !m_touch->canBeMouse)
           || (output->arguments.tap.count == 2)))
      || (output->inputType == PT_PEN &&
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

void WinWindow::waitTimerToConvertFingerAsMouseMovement()
{
  ASSERT(m_touch);
  m_touch->canBeMouse = true;
  clearDelayedTouchEvents();
  SetTimer(m_hwnd, m_touch->timerID = 1,
           kFingerAsMouseTimeout, nullptr);
  TOUCH_TRACE(" - Set timer\n");
}

void WinWindow::convertFingerAsMouseMovement()
{
  ASSERT(m_touch);
  m_touch->asMouse = true;
  sendDelayedTouchEvents();
}

void WinWindow::delegateFingerToInteractionContext()
{
  ASSERT(m_touch);
  m_touch->canBeMouse = false;
  m_touch->asMouse = false;
  clearDelayedTouchEvents();
  if (m_touch->timerID > 0)
    killTouchTimer();
}

void WinWindow::sendDelayedTouchEvents()
{
  ASSERT(m_touch);
  for (auto& ev : m_touch->delayedEvents)
    queueEvent(ev);
  clearDelayedTouchEvents();
}

void WinWindow::clearDelayedTouchEvents()
{
  ASSERT(m_touch);
  m_touch->delayedEvents.clear();
}

void WinWindow::killTouchTimer()
{
  ASSERT(m_touch);
  if (m_touch->timerID > 0) {
    KillTimer(m_hwnd, m_touch->timerID);
    m_touch->timerID = 0;
    TOUCH_TRACE(" - Kill timer\n");
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
