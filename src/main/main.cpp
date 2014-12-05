/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/app_options.h"
#include "app/console.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "base/exception.h"
#include "base/memory.h"
#include "base/memory_dump.h"
#include "base/system_console.h"
#include "she/error.h"
#include "she/scoped_handle.h"
#include "she/system.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

namespace {

  // Memory leak detector wrapper
  class MemLeak {
  public:
#ifdef MEMLEAK
    MemLeak() { base_memleak_init(); }
    ~MemLeak() { base_memleak_exit(); }
#else
    MemLeak() { }
#endif
  };

}

// Aseprite entry point. (Called from she library.)
int app_main(int argc, char* argv[])
{
  // Initialize the random seed.
  std::srand(static_cast<unsigned int>(std::time(NULL)));

#ifdef WIN32
  ::CoInitialize(NULL);
#endif

  try {
    base::MemoryDump memoryDump;
    MemLeak memleak;
    base::SystemConsole systemConsole;
    app::AppOptions options(argc, const_cast<const char**>(argv));
    she::ScopedHandle<she::System> system(she::create_system());
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
    she::error_message(e.what());
    return 1;
  }
}
