// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she.h"

#include "base/compiler_specific.h"
#include "base/concurrent_queue.h"
#include "base/string.h"

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

class Alleg4Surface : public Surface
                    , public LockedSurface {
public:
  enum DestroyFlag { NoDestroy, AutoDestroy };

  Alleg4Surface(BITMAP* bmp, DestroyFlag destroy)
    : m_bmp(bmp)
    , m_destroy(destroy)
  {
  }

  Alleg4Surface(int width, int height)
    : m_bmp(create_bitmap(width, height))
    , m_destroy(AutoDestroy)
  {
  }

  ~Alleg4Surface() {
    if (m_destroy == AutoDestroy)
      destroy_bitmap(m_bmp);
  }

  // Surface implementation

  void dispose() {
    delete this;
  }

  int width() const {
    return m_bmp->w;
  }

  int height() const {
    return m_bmp->h;
  }

  LockedSurface* lock() {
    acquire_bitmap(m_bmp);
    return this;
  }

  void* nativeHandle() {
    return reinterpret_cast<void*>(m_bmp);
  }

  // LockedSurface implementation

  void unlock() {
    release_bitmap(m_bmp);
  }

  void clear() {
    clear_to_color(m_bmp, 0);
  }

  void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const {
    ASSERT(m_bmp);
    ASSERT(dest);
    ASSERT(static_cast<Alleg4Surface*>(dest)->m_bmp);

    blit(m_bmp,
         static_cast<Alleg4Surface*>(dest)->m_bmp,
         srcx, srcy,
         dstx, dsty,
         width, height);
  }

  void drawAlphaSurface(const LockedSurface* src, int dstx, int dsty) {
    set_alpha_blender();
    draw_trans_sprite(m_bmp, static_cast<const Alleg4Surface*>(src)->m_bmp, dstx, dsty);
  }

private:
  BITMAP* m_bmp;
  DestroyFlag m_destroy;
};

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

  }
  return ::CallWindowProc(base_wndproc, hwnd, msg, wparam, lparam);
}

void subclass_hwnd(HWND hwnd)
{
  SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_HSCROLL | WS_VSCROLL);
  SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);

  SCROLLINFO si;
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS | SIF_RANGE;
  si.nMin = 0;
  si.nPos = 50;
  si.nMax = 100;
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
    Surface* newSurface = new Alleg4Surface(SCREEN_W/m_scale,
                                            SCREEN_H/m_scale);
    if (m_surface)
      m_surface->dispose();
    m_surface = newSurface;
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
  Alleg4System() {
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

  void dispose() {
    delete this;
  }

  Capabilities capabilities() const {
    return (Capabilities)
      (kCanResizeDisplayCapability
#ifdef WIN32
        | kMouseEventsCapability
#endif
       );
  }

  Display* createDisplay(int width, int height, int scale) {
    return new Alleg4Display(width, height, scale);
  }

  Surface* createSurface(int width, int height) {
    return new Alleg4Surface(width, height);
  }

  Surface* createSurfaceFromNativeHandle(void* nativeHandle) {
    return new Alleg4Surface(reinterpret_cast<BITMAP*>(nativeHandle),
                             Alleg4Surface::AutoDestroy);
  }

  Clipboard* createClipboard() {
    return new ClipboardImpl();
  }

};

static System* g_instance;

System* CreateSystem() {
  return g_instance = new Alleg4System();
}

System* Instance()
{
  return g_instance;
}

}

// It must be defined by the user program code.
extern int app_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  return app_main(argc, argv);
}

END_OF_MAIN();
