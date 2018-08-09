// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/console.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "base/exception.h"
#include "base/memory.h"
#include "base/memory_dump.h"
#include "base/system_console.h"
#include "os/error.h"
#include "os/scoped_handle.h"
#include "os/system.h"

#include <clocale>
#include <cstdlib>
#include <ctime>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#endif

namespace {

  // Memory leak detector wrapper
  class MemLeak {
  public:
#ifdef LAF_MEMLEAK
    MemLeak() { base_memleak_init(); }
    ~MemLeak() { base_memleak_exit(); }
#else
    MemLeak() { }
#endif
  };

}

// Aseprite entry point. (Called from "os" library.)
int app_main(int argc, char* argv[])
{
  // Initialize the locale. Aseprite isn't ready to handle numeric
  // fields with other locales (e.g. we expect strings like "10.32" be
  // used in std::strtod(), not something like "10,32").
  std::setlocale(LC_ALL, "en-US");
  ASSERT(std::strtod("10.32", nullptr) == 10.32);

  // Initialize the random seed.
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

#ifdef _WIN32
  ::CoInitialize(NULL);
#endif

  try {
    base::MemoryDump memoryDump;
    MemLeak memleak;
    base::SystemConsole systemConsole;
    app::AppOptions options(argc, const_cast<const char**>(argv));
    os::ScopedHandle<os::System> system(os::create_system());
    app::App app;

    // Change the name of the memory dump file
    {
      std::string filename = app::memory_dump_filename();
      if (!filename.empty())
        memoryDump.setFileName(filename);
    }

    app.initialize(options);

    if (options.startShell())
      systemConsole.prepareShell();

    app.run();
    return 0;
  }
  catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    os::error_message(e.what());
    return 1;
  }
}
