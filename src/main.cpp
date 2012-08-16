/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "app.h"
#include "base/exception.h"
#include "base/memory.h"
#include "base/memory_dump.h"
#include "console.h"
#include "resource_finder.h"
#include "she/she.h"
#include "ui/base.h"

#include <cstdlib>
#include <ctime>

#ifdef WIN32
#include <windows.h>
#endif

//////////////////////////////////////////////////////////////////////
// Information for "ident".

const char aseprite_ident[] =
    "$" PACKAGE ": " VERSION " " COPYRIGHT " $\n"
    "$Website: " WEBSITE " $\n";

//////////////////////////////////////////////////////////////////////
// Memory leak detector wrapper

class MemLeak
{
public:
  MemLeak() {
#ifdef MEMLEAK
    base_memleak_init();
#endif
  }
  ~MemLeak() {
#ifdef MEMLEAK
    base_memleak_exit();
#endif
  }
};

static bool get_memory_dump_filename(std::string& filename)
{
#ifdef WIN32
  ResourceFinder rf;
  rf.findInBinDir("aseprite-memory.dmp");
  filename = rf.first();
  return true;
#else
  return false;
#endif
}

// ASEPRITE entry point
int main(int argc, char* argv[])
{
  // Initialize the random seed.
  std::srand(static_cast<unsigned int>(std::time(NULL)));

#ifdef WIN32
  CoInitialize(NULL);
#endif

  base::MemoryDump memoryDump;
  she::ScopedHandle<she::System> system(she::CreateSystem());
  MemLeak memleak;
  ui::GuiSystem guiSystem;
  App app(argc, argv);

  // Change the name of the memory dump file
  {
    std::string filename;
    if (get_memory_dump_filename(filename))
      memoryDump.setFileName(filename);
  }

  return app.run();
}
