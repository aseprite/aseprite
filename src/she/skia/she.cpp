// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/memory.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "she/she.h"

#include "she/skia/skia_system.h"

#if __APPLE__
  #include "she/osx/app.h"
  #include <CoreServices/CoreServices.h>
#elif !defined(_WIN32)
  #include "she/x11/x11.h"
#endif

namespace she {

static System* g_instance;

System* create_system() {
  return g_instance = new SkiaSystem();
}

System* instance()
{
  return g_instance;
}

void error_message(const char* msg)
{
  fputs(msg, stderr);
  // TODO
}

void clear_keyboard_buffer()
{
  // Do nothing
}

} // namespace she

extern int app_main(int argc, char* argv[]);

#if _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR lpCmdLine, int nCmdShow) {
  int argc = __argc;
  char** argv;
  if (__wargv && argc > 0) {
    argv = new char*[argc];
    for (int i=0; i<argc; ++i)
      argv[i] = base_strdup(base::to_utf8(std::wstring(__wargv[i])).c_str());
  }
  else {
    argv = new char*[1];
    argv[0] = base_strdup("");
    argc = 1;
  }
#else
int main(int argc, char* argv[]) {
#endif

#if __APPLE__
  she::OSXApp app;
  if (!app.init())
    return 1;
#elif !defined(_WIN32)
  she::X11 x11;
#endif

  return app_main(argc, argv);
}
