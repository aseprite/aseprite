// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "she/alleg4/alleg_display.h"
#include "she/alleg4/alleg_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#ifdef _WIN32
  #include <winalleg.h>
  #include <windowsx.h>
  #include <commctrl.h>

  #if defined STRICT || defined __GNUC__
    typedef WNDPROC wndproc_t;
  #else
    typedef FARPROC wndproc_t;
  #endif

  #ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL 0x020E
  #endif

#elif defined ALLEGRO_UNIX

  #include <xalleg.h>
  #ifdef None
  #undef None
  #define X11_None 0L
  #endif

#endif

#include "loadpng.h"

#include <cassert>
#include <list>
#include <sstream>
#include <vector>

#include "she/alleg4/display_events.h"

#ifdef USE_KEY_POLLER
  #include "she/alleg4/key_poller.h"
#endif

#ifdef USE_MOUSE_POLLER
  #include "she/alleg4/mouse_poller.h"
#endif

void* get_osx_window();

namespace she {

Alleg4Display* unique_display = NULL;
int display_scale;

namespace {

#if _WIN32

wndproc_t base_wndproc = NULL;
bool display_has_mouse = false;
bool capture_mouse = false;

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // Don't process DDE messages here because we cannot get the first
  // DDE message correctly. The problem is that the message pump
  // starts before we can subclass the Allegro HWND, so we don't have
  // enough time to inject the code to process this kind of message.
  //
  // For more info see "Once you go input-idle, your application is
  // deemed ready to receive DDE messages":
  //   https://blogs.msdn.microsoft.com/oldnewthing/20140620-00/?p=693
  //
  // Anyway a possible solution would be to avoid processing the
  // message loop until we subclass the HWND. I've tested this and it
  // doesn't work, maybe because the message pump on Allegro 4 isn't
  // in the main thread, I really don't know. But it just crash the
  // whole system (Windows 10).

  switch (msg) {

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
      queue_event(ev);
      break;
    }

#ifndef USE_MOUSE_POLLER

    case WM_MOUSEMOVE: {
      // Adjust capture
      if (capture_mouse) {
        if (GetCapture() != hwnd)
          SetCapture(hwnd);
      }
      else {
        if (GetCapture() == hwnd)
          ReleaseCapture();
      }

      //LOG("GetCapture=%p hwnd=%p\n", GetCapture(), hwnd);

      Event ev;
      ev.setPosition(gfx::Point(
          GET_X_LPARAM(lparam) / display_scale,
          GET_Y_LPARAM(lparam) / display_scale));

      if (!display_has_mouse) {
        display_has_mouse = true;

        ev.setType(Event::MouseEnter);
        queue_event(ev);

        // Track mouse to receive WM_MOUSELEAVE message.
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        _TrackMouseEvent(&tme);
      }

      ev.setType(Event::MouseMove);
      queue_event(ev);
      break;
    }

    case WM_NCMOUSEMOVE:
    case WM_MOUSELEAVE:
      if (display_has_mouse) {
        display_has_mouse = false;

        Event ev;
        ev.setType(Event::MouseLeave);
        queue_event(ev);
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
      Event ev;
      ev.setType(Event::MouseDown);
      ev.setPosition(gfx::Point(
          GET_X_LPARAM(lparam) / display_scale,
          GET_Y_LPARAM(lparam) / display_scale));
      ev.setButton(
        msg == WM_LBUTTONDOWN ? Event::LeftButton:
        msg == WM_RBUTTONDOWN ? Event::RightButton:
        msg == WM_MBUTTONDOWN ? Event::MiddleButton: Event::NoneButton);
      queue_event(ev);
      break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
      Event ev;
      ev.setType(Event::MouseUp);
      ev.setPosition(gfx::Point(
          GET_X_LPARAM(lparam) / display_scale,
          GET_Y_LPARAM(lparam) / display_scale));
      ev.setButton(
        msg == WM_LBUTTONUP ? Event::LeftButton:
        msg == WM_RBUTTONUP ? Event::RightButton:
        msg == WM_MBUTTONUP ? Event::MiddleButton: Event::NoneButton);
      queue_event(ev);

      // Avoid popup menu for scrollbars
      if (msg == WM_RBUTTONUP)
        return 0;

      break;
    }

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK: {
      Event ev;
      ev.setType(Event::MouseDoubleClick);
      ev.setPosition(gfx::Point(
          GET_X_LPARAM(lparam) / display_scale,
          GET_Y_LPARAM(lparam) / display_scale));
      ev.setButton(
        msg == WM_LBUTTONDBLCLK ? Event::LeftButton:
        msg == WM_RBUTTONDBLCLK ? Event::RightButton:
        msg == WM_MBUTTONDBLCLK ? Event::MiddleButton: Event::NoneButton);
      queue_event(ev);
      break;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      POINT pos = { GET_X_LPARAM(lparam),
                    GET_Y_LPARAM(lparam) };
      ScreenToClient(hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setPosition(gfx::Point(pos.x, pos.y) / display_scale);

      int z = ((short)HIWORD(wparam)) / WHEEL_DELTA;
      gfx::Point delta(
        (msg == WM_MOUSEHWHEEL ? z: 0),
        (msg == WM_MOUSEWHEEL ? -z: 0));
      ev.setWheelDelta(delta);

      //LOG("WHEEL: %d %d\n", delta.x, delta.y);

      queue_event(ev);
      break;
    }

    case WM_HSCROLL:
    case WM_VSCROLL: {
      POINT pos;
      GetCursorPos(&pos);
      ScreenToClient(hwnd, &pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setPosition(gfx::Point(pos.x, pos.y) / display_scale);

      int bar = (msg == WM_HSCROLL ? SB_HORZ: SB_VERT);
      int z = GetScrollPos(hwnd, bar);

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

      //LOG("SCROLL: %d %d\n", delta.x, delta.y);

      SetScrollPos(hwnd, bar, 50, FALSE);

      queue_event(ev);
      break;
    }

#endif

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
      LRESULT result = ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);
      gfx::Point pt(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

      RECT rc;
      ::GetClientRect(hwnd, &rc);
      ::MapWindowPoints(hwnd, NULL, (POINT*)&rc, 2);
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

  }
  return ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);
}

void subclass_hwnd(HWND hwnd)
{
  SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_HSCROLL | WS_VSCROLL);
  SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);

  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
  si.nMin = 0;
  si.nPos = 50;
  si.nMax = 100;
  si.nPage = 10;
  SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);
  SetScrollInfo(hwnd, SB_VERT, &si, FALSE);

  base_wndproc = (wndproc_t)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndproc);
}

void unsubclass_hwnd(HWND hwnd)
{
  SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)base_wndproc);
  base_wndproc = NULL;
}

#endif // _WIN32

} // anonymous namespace

Alleg4Display::Alleg4Display(int width, int height, int scale)
  : m_surface(NULL)
  , m_scale(0)
  , m_nativeCursor(kNoCursor)
  , m_restoredWidth(0)
  , m_restoredHeight(0)
{
  unique_display = this;

  if (install_mouse() < 0) throw DisplayCreationException(allegro_error);
  if (install_keyboard() < 0) throw DisplayCreationException(allegro_error);

#ifdef FULLSCREEN_PLATFORM
  set_color_depth(16);        // TODO Try all color depths for fullscreen platforms
#else
  set_color_depth(desktop_color_depth());
#endif

  if (set_gfx_mode(
#ifdef FULLSCREEN_PLATFORM
        GFX_AUTODETECT_FULLSCREEN,
#else
        GFX_AUTODETECT_WINDOWED,
#endif
        width, height, 0, 0) < 0)
    throw DisplayCreationException(allegro_error);

  show_mouse(NULL);
  setScale(scale);

#if _WIN32
  subclass_hwnd((HWND)nativeHandle());
#endif
}

Alleg4Display::~Alleg4Display()
{
  unique_display = NULL;

#if _WIN32
  unsubclass_hwnd((HWND)nativeHandle());
#endif

  m_surface->dispose();
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

void Alleg4Display::dispose()
{
  delete this;
}

int Alleg4Display::width() const
{
  return SCREEN_W;
}

int Alleg4Display::height() const
{
  return SCREEN_H;
}

int Alleg4Display::originalWidth() const
{
  return m_restoredWidth > 0 ? m_restoredWidth: width();
}

int Alleg4Display::originalHeight() const
{
  return m_restoredHeight > 0 ? m_restoredHeight: height();
}

void Alleg4Display::setOriginalWidth(int width)
{
  m_restoredWidth = width;
}

void Alleg4Display::setOriginalHeight(int height)
{
  m_restoredHeight = height;
}

int Alleg4Display::scale() const
{
  return m_scale;
}

void Alleg4Display::setScale(int scale)
{
  ASSERT(scale >= 1);
  display_scale = scale;

  if (m_scale == scale)
    return;

  m_scale = scale;
  recreateSurface();
}

void Alleg4Display::recreateSurface()
{
  Surface* newSurface = new Alleg4Surface(
    SCREEN_W / m_scale,
    SCREEN_H / m_scale, Alleg4Surface::DeleteAndDestroy);
  if (m_surface)
    m_surface->dispose();
  m_surface = newSurface;
}

Surface* Alleg4Display::getSurface()
{
  return m_surface;
}

void Alleg4Display::flip(const gfx::Rect& bounds)
{
  if (is_display_resize_awaiting())
    return;

  BITMAP* bmp = reinterpret_cast<BITMAP*>(m_surface->nativeHandle());
  if (m_scale == 1) {
    blit(bmp, screen,
         bounds.x, bounds.y,
         bounds.x, bounds.y,
         bounds.w, bounds.h);
  }
  else {
    stretch_blit(bmp, screen,
                 bounds.x, bounds.y, bounds.w, bounds.h,
                 bounds.x*m_scale, bounds.y*m_scale,
                 bounds.w*m_scale, bounds.h*m_scale);
  }
}

void Alleg4Display::maximize()
{
#ifdef _WIN32
  ::ShowWindow(win_get_window(), SW_MAXIMIZE);
#endif
}

bool Alleg4Display::isMaximized() const
{
#ifdef _WIN32
  return (::GetWindowLong(win_get_window(), GWL_STYLE) & WS_MAXIMIZE ? true: false);
#else
  return false;
#endif
}

bool Alleg4Display::isMinimized() const
{
#ifdef _WIN32
  return (::GetWindowLong(win_get_window(), GWL_STYLE) & WS_MINIMIZE ? true: false);
#else
  return false;
#endif
}

void Alleg4Display::setTitleBar(const std::string& title)
{
  set_window_title(title.c_str());
}

NativeCursor Alleg4Display::nativeMouseCursor()
{
  return m_nativeCursor;
}

bool Alleg4Display::setNativeMouseCursor(NativeCursor cursor)
{
  int newCursor = MOUSE_CURSOR_NONE;

  switch (cursor) {
    case kNoCursor:
      newCursor = MOUSE_CURSOR_NONE;
      break;
    case kArrowCursor:
      newCursor = MOUSE_CURSOR_ARROW;
      break;
    case kIBeamCursor:
      newCursor = MOUSE_CURSOR_EDIT;
      break;
    case kWaitCursor:
      newCursor = MOUSE_CURSOR_BUSY;
      break;
    case kHelpCursor:
      newCursor = MOUSE_CURSOR_QUESTION;
      break;
#ifdef ALLEGRO4_WITH_EXTRA_CURSORS
    case kForbiddenCursor: newCursor = MOUSE_CURSOR_FORBIDDEN; break;
    case kMoveCursor: newCursor = MOUSE_CURSOR_MOVE; break;
    case kLinkCursor: newCursor = MOUSE_CURSOR_LINK; break;
    case kSizeNSCursor: newCursor = MOUSE_CURSOR_SIZE_NS; break;
    case kSizeWECursor: newCursor = MOUSE_CURSOR_SIZE_WE; break;
    case kSizeNCursor: newCursor = MOUSE_CURSOR_SIZE_N; break;
    case kSizeNECursor: newCursor = MOUSE_CURSOR_SIZE_NE; break;
    case kSizeECursor: newCursor = MOUSE_CURSOR_SIZE_E; break;
    case kSizeSECursor: newCursor = MOUSE_CURSOR_SIZE_SE; break;
    case kSizeSCursor: newCursor = MOUSE_CURSOR_SIZE_S; break;
    case kSizeSWCursor: newCursor = MOUSE_CURSOR_SIZE_SW; break;
    case kSizeWCursor: newCursor = MOUSE_CURSOR_SIZE_W; break;
    case kSizeNWCursor: newCursor = MOUSE_CURSOR_SIZE_NW; break;
#endif
    default:
      return false;
  }

  m_nativeCursor = cursor;
  return (show_os_cursor(newCursor) == 0);
}

void Alleg4Display::setMousePosition(const gfx::Point& position)
{
  position_mouse(
    m_scale * position.x,
    m_scale * position.y);
}

void Alleg4Display::captureMouse()
{
#ifdef _WIN32

  capture_mouse = true;

#elif defined(ALLEGRO_UNIX)

  XGrabPointer(_xwin.display, _xwin.window, False,
               PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync,
               X11_None, X11_None, CurrentTime);

#endif
}

void Alleg4Display::releaseMouse()
{
#ifdef _WIN32

  capture_mouse = false;

#elif defined(ALLEGRO_UNIX)

  XUngrabPointer(_xwin.display, CurrentTime);

#endif
}

std::string Alleg4Display::getLayout()
{
#ifdef _WIN32
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);
  if (GetWindowPlacement((HWND)nativeHandle(), &wp)) {
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
#endif
  return "";
}

void Alleg4Display::setLayout(const std::string& layout)
{
#ifdef _WIN32

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

  if (SetWindowPlacement((HWND)nativeHandle(), &wp)) {
    // TODO use the return value
  }
#else
  // Do nothing
#endif
}

void* Alleg4Display::nativeHandle()
{
#ifdef _WIN32
  return reinterpret_cast<void*>(win_get_window());
#elif defined __APPLE__
  return get_osx_window();
#else
  return nullptr;
#endif
}

} // namespace she
