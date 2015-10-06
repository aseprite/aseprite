// SHE library
// Copyright (C) 2012-2015  David Capello
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
#include <vector>

#include "she/alleg4/clock.h"
#include "she/alleg4/display_events.h"
#ifdef USE_KEY_POLLER
  #include "she/alleg4/key_poller.h"
#endif
#ifdef USE_MOUSE_POLLER
  #include "she/alleg4/mouse_poller.h"
#endif

static she::System* g_instance = nullptr;

namespace she {

class Alleg4EventQueue : public EventQueue {
public:
  static Alleg4EventQueue g_queue;

  void getEvent(Event& event, bool canWait) override {
    (void)canWait;              // Ignore this parameter

    display_generate_events();

#ifdef USE_KEY_POLLER
    key_poller_generate_events();
#endif

#ifdef USE_MOUSE_POLLER
    mouse_poller_generate_events();
#endif

    if (!m_events.try_pop(event))
      event.setType(Event::None);
  }

  void queueEvent(const Event& event) override {
    m_events.push(event);
  }

private:
  // We need a concurrent queue because events are generated in one
  // thread (the thread created by Allegro 4 for the HWND), and
  // consumed in the other thread (the main/program logic thread).
  base::concurrent_queue<Event> m_events;
};

Alleg4EventQueue g_queue;

EventQueue* EventQueue::instance() {
  return &g_queue;
}

class Alleg4System : public CommonSystem {
public:
  Alleg4System()
  {
    if (allegro_init() < 0)
      throw base::Exception("Cannot initialize Allegro library: %s", allegro_error);

    set_uformat(U_UTF8);
#if MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION) >= MAKE_VERSION(4, 4, 0)
    _al_detect_filename_encoding();
#endif
    install_timer();

    // Register PNG as a supported bitmap type
    register_bitmap_file_type("png", load_png, save_png);

    // Init event sources
    clock_init();
    display_events_init();
#ifdef USE_KEY_POLLER
    key_poller_init();
#endif
#ifdef USE_MOUSE_POLLER
    mouse_poller_init();
#endif

    g_instance = this;
  }

  ~Alleg4System() {
    clock_exit();
    remove_timer();
    allegro_exit();

    g_instance = nullptr;
  }

  void dispose() override {
    delete this;
  }

  Capabilities capabilities() const override {
    return (Capabilities)(Capabilities::CanResizeDisplay);
  }

  EventQueue* eventQueue() override { // TODO remove this function
    return EventQueue::instance();
  }

  bool gpuAcceleration() const override {
    return true;
  }

  void setGpuAcceleration(bool state) override {
    // Do nothing
  }

  Display* defaultDisplay() override {
    return unique_display;
  }

  Display* createDisplay(int width, int height, int scale) override {
    //LOG("Creating display %dx%d (scale = %d)\n", width, height, scale);
    return new Alleg4Display(width, height, scale);
  }

  Surface* createSurface(int width, int height) override {
    return new Alleg4Surface(width, height, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* createRgbaSurface(int width, int height) override {
    return new Alleg4Surface(width, height, 32, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* loadSurface(const char* filename) override {
    PALETTE pal;
    BITMAP* bmp = load_bitmap(filename, pal);
    if (!bmp)
      throw std::runtime_error("Error loading image");

    return new Alleg4Surface(bmp, Alleg4Surface::DeleteAndDestroy);
  }

  Surface* loadRgbaSurface(const char* filename) override {
    int old_color_conv = _color_conv;
    set_color_conversion(COLORCONV_NONE);
    Surface* sur = loadSurface(filename);
    set_color_conversion(old_color_conv);
    return sur;
  }
};

System* create_system() {
  return new Alleg4System();
}

System* instance()
{
  return g_instance;
}

void error_message(const char* msg)
{
  if (g_instance && g_instance->logger())
    g_instance->logger()->logError(msg);

#ifdef _WIN32
  std::wstring wmsg = base::from_utf8(msg);
  std::wstring title = base::from_utf8(PACKAGE);
  ::MessageBoxW(NULL, wmsg.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#else
  allegro_message("%s", msg);
#endif
}

bool is_key_pressed(KeyScancode scancode)
{
  return key[scancode] ? true: false;
}

void clear_keyboard_buffer()
{
  clear_keybuf();
}

} // namespace she

// It must be defined by the user program code.
extern int app_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  return app_main(argc, argv);
}

END_OF_MAIN();
