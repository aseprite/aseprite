// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/rect.h"
#include "gfx/size.h"
#include "she/she.h"

#include "she/common/system.h"

#include "she/skia/skia_system.h"

#if __APPLE__
  #include "she/osx/app.h"
  #include <CoreServices/CoreServices.h>
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
  // TODO
}

#ifndef _WIN32
bool is_key_pressed(KeyScancode scancode)
{
  return false; // Do nothing
}
#endif

void clear_keyboard_buffer()
{
  // Do nothing
}

int clock_value()
{
  // TODO
#if _WIN32
  return (int)GetTickCount();
#elif defined(__APPLE__)
  return TickCount();
#else
  return 0;
#endif
}

} // namespace she

extern int app_main(int argc, char* argv[]);

#if _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
  int argc = 1;
  char* argv[] = { "" };
#else
int main(int argc, char* argv[])
{
#endif

#if __APPLE__
  she::OSXApp app;
  return app.run(argc, argv);
#else
  return app_main(argc, argv);
#endif
}
