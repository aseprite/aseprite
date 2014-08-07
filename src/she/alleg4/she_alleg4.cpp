// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/compiler_specific.h"
#include "base/concurrent_queue.h"
#include "base/string.h"
#include "she/alleg4/alleg4_font.h"
#include "she/alleg4/alleg4_surface.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#ifdef WIN32
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
#endif

#ifdef WIN32
  #include "she/clipboard_win.h"
#else
  #include "she/clipboard_simple.h"
#endif

#include "loadpng.h"

#include <cassert>
#include <vector>
#include <list>

#define DISPLAY_FLAG_FULL_REFRESH     1
#define DISPLAY_FLAG_WINDOW_RESIZE    2

static volatile int display_flags = 0;
static volatile int original_width = 0;
static volatile int original_height = 0;

// Used by set_display_switch_callback(SWITCH_IN, ...).
static void display_switch_in_callback()
{
  display_flags |= DISPLAY_FLAG_FULL_REFRESH;
}

END_OF_STATIC_FUNCTION(display_switch_in_callback);

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
// Called when the window is resized
static void resize_callback(RESIZE_DISPLAY_EVENT* ev)
{
  if (ev->is_maximized) {
    original_width = ev->old_w;
    original_height = ev->old_h;
  }
  display_flags |= DISPLAY_FLAG_WINDOW_RESIZE;
}
#endif // ALLEGRO4_WITH_RESIZE_PATCH

namespace she {

class Alleg4EventQueue : public EventQueue {
public:
  Alleg4EventQueue() {
  }

  void dispose() {
    delete this;
  }

  void getEvent(Event& event) {
    if (!m_events.try_pop(event))
      event.setType(Event::None);
  }

  void queueEvent(const Event& event) {
    m_events.push(event);
  }

private:
  // We need a concurrent queue because events are generated in one
  // thread (the thread created by Allegro 4 for the HWND), and
  // consumed in the other thread (the main/program logic thread).
  base::concurrent_queue<Event> m_events;
};

namespace {

base::mutex unique_display_mutex;
Display* unique_display = NULL;
int display_scale;

#if WIN32

wndproc_t base_wndproc = NULL;
bool display_has_mouse = false;

static void queue_event(Event& ev)
{
  base::scoped_lock hold(unique_display_mutex);
  if (unique_display)
    static_cast<Alleg4EventQueue*>(unique_display->getEventQueue())->queueEvent(ev);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
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

    case WM_MOUSEMOVE: {
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

    case WM_MOUSELEAVE: {
      display_has_mouse = false;

      Event ev;
      ev.setType(Event::MouseLeave);
      queue_event(ev);
      break;
    }

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
      RECT rc;
      ::GetWindowRect(hwnd, &rc);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setPosition((gfx::Point(
            GET_X_LPARAM(lparam),
            GET_Y_LPARAM(lparam)) - gfx::Point(rc.left, rc.top))
        / display_scale);

      int z = ((short)HIWORD(wparam)) / WHEEL_DELTA;
      gfx::Point delta(
        (msg == WM_MOUSEHWHEEL ? z: 0),
        (msg == WM_MOUSEWHEEL ? -z: 0));
      ev.setWheelDelta(delta);

      //PRINTF("WHEEL: %d %d\n", delta.x, delta.y);

      queue_event(ev);
      break;
    }

    case WM_HSCROLL:
    case WM_VSCROLL: {
      RECT rc;
      ::GetWindowRect(hwnd, &rc);

      POINT pos;
      ::GetCursorPos(&pos);

      Event ev;
      ev.setType(Event::MouseWheel);
      ev.setPosition((gfx::Point(pos.x, pos.y) - gfx::Point(rc.left, rc.top))
        / display_scale);

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

      //PRINTF("SCROLL: %d %d\n", delta.x, delta.y);

      SetScrollPos(hwnd, bar, 50, FALSE);

      queue_event(ev);
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
      LRESULT result = ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);

      // We ignore scrollbars so if the mouse is above them, we return
      // as it's in the window resize area. (Remember that we have
      // scroll bars are enabled and visible to receive trackpad
      // messages only.)
      if (result == HTHSCROLL)
        result = HTBOTTOM;
      else if (result == HTVSCROLL)
        result = HTRIGHT;

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
  
#endif
} // anonymous namespace

class Alleg4Display : public Display {
public:
  Alleg4Display(int width, int height, int scale)
    : m_surface(NULL)
    , m_scale(0) {
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

    setScale(scale);

    m_queue = new Alleg4EventQueue();

    // Add a hook to display-switch so when the user returns to the
    // screen it's completelly refreshed/redrawn.
    LOCK_VARIABLE(display_flags);
    LOCK_FUNCTION(display_switch_in_callback);
    set_display_switch_callback(SWITCH_IN, display_switch_in_callback);

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
    // Setup the handler for window-resize events
    set_resize_callback(resize_callback);
#endif

#if WIN32
    subclass_hwnd((HWND)nativeHandle());
#endif
  }

  ~Alleg4Display() {
    // Put "unique_display" to null so queue_event() doesn't use
    // "m_queue" anymore.
    {
      base::scoped_lock hold(unique_display_mutex);
      unique_display = NULL;
    }

#if WIN32
    unsubclass_hwnd((HWND)nativeHandle());
#endif

    delete m_queue;

    m_surface->dispose();
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
  }

  void dispose() OVERRIDE {
    delete this;
  }

  int width() const OVERRIDE {
    return SCREEN_W;
  }

  int height() const OVERRIDE {
    return SCREEN_H;
  }

  int originalWidth() const OVERRIDE {
    return original_width > 0 ? original_width: width();
  }

  int originalHeight() const OVERRIDE {
    return original_height > 0 ? original_height: height();
  }

  void setScale(int scale) OVERRIDE {
    ASSERT(scale >= 1);
    display_scale = scale;

    if (m_scale == scale)
      return;

    m_scale = scale;
    Surface* newSurface = new Alleg4Surface(
      SCREEN_W / m_scale,
      SCREEN_H / m_scale, Alleg4Surface::DeleteAndDestroy);
    if (m_surface)
      m_surface->dispose();
    m_surface = newSurface;
  }

  int scale() const OVERRIDE {
    return m_scale;
  }

  NonDisposableSurface* getSurface() OVERRIDE {
    return static_cast<NonDisposableSurface*>(m_surface);
  }

  bool flip() OVERRIDE {
#ifdef ALLEGRO4_WITH_RESIZE_PATCH
    if (display_flags & DISPLAY_FLAG_WINDOW_RESIZE) {
      display_flags ^= DISPLAY_FLAG_WINDOW_RESIZE;

      acknowledge_resize();

      int scale = m_scale;
      m_scale = 0;
      setScale(scale);
      return false;
    }
#endif

    BITMAP* bmp = reinterpret_cast<BITMAP*>(m_surface->nativeHandle());
    if (m_scale == 1) {
      blit(bmp, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    }
    else {
      stretch_blit(bmp, screen,
                   0, 0, bmp->w, bmp->h,
                   0, 0, SCREEN_W, SCREEN_H);
    }

    return true;
  }

  void maximize() OVERRIDE {
#ifdef WIN32
    ::ShowWindow(win_get_window(), SW_MAXIMIZE);
#endif
  }

  bool isMaximized() const OVERRIDE {
#ifdef WIN32
    return (::GetWindowLong(win_get_window(), GWL_STYLE) & WS_MAXIMIZE ? true: false);
#else
    return false;
#endif
  }

  EventQueue* getEventQueue() OVERRIDE {
    return m_queue;
  }

  void setMousePosition(const gfx::Point& position) OVERRIDE {
    position_mouse(
      m_scale * position.x,
      m_scale * position.y);
  }

  void* nativeHandle() OVERRIDE {
#ifdef WIN32
    return reinterpret_cast<void*>(win_get_window());
#else
    return NULL;
#endif
  }

private:
  Surface* m_surface;
  int m_scale;
  Alleg4EventQueue* m_queue;
};

class Alleg4System : public System {
public:
  Alleg4System()
    : m_font(font, Alleg4Font::None)       // Default Allegro font
  {
    allegro_init();
    set_uformat(U_UTF8);
    _al_detect_filename_encoding();
    install_timer();

    // Register PNG as a supported bitmap type
    register_bitmap_file_type("png", load_png, save_png);
  }

  ~Alleg4System() {
    remove_timer();
    allegro_exit();
  }

  void dispose() OVERRIDE {
    delete this;
  }

  Capabilities capabilities() const OVERRIDE {
    return (Capabilities)
      (kCanResizeDisplayCapability
#ifdef WIN32
        | kMouseEventsCapability
#endif
       );
  }

  Display* defaultDisplay() OVERRIDE {
    return unique_display;
  }

  Font* defaultFont() OVERRIDE {
    return &m_font;
  }

  Display* createDisplay(int width, int height, int scale) OVERRIDE {
    return new Alleg4Display(width, height, scale);
  }

  Surface* createSurface(int width, int height) OVERRIDE {
    return new Alleg4Surface(width, height, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* createRgbaSurface(int width, int height) OVERRIDE {
    return new Alleg4Surface(width, height, 32, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* loadSurface(const char* filename) OVERRIDE {
    PALETTE pal;
    BITMAP* bmp = load_bitmap(filename, pal);
    if (!bmp)
      throw std::runtime_error("Error loading image");

    return new Alleg4Surface(bmp, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* loadRgbaSurface(const char* filename) OVERRIDE {
    int old_color_conv = _color_conv;
    set_color_conversion(COLORCONV_NONE);
    Surface* sur = loadSurface(filename);
    set_color_conversion(old_color_conv);
    return sur;
  }

  Clipboard* createClipboard() OVERRIDE {
    return new ClipboardImpl();
  }

private:
  Alleg4Font m_font;
};

static System* g_instance;

System* create_system() {
  return g_instance = new Alleg4System();
}

System* instance()
{
  return g_instance;
}

void error_message(const char* msg)
{
#ifdef WIN32
  std::wstring wmsg = base::from_utf8(msg);
  std::wstring title = base::from_utf8(PACKAGE);
  ::MessageBoxW(NULL, wmsg.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#else
  allegro_message(msg);
#endif
}

} // namespace she

// It must be defined by the user program code.
extern int app_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  return app_main(argc, argv);
}

END_OF_MAIN();
