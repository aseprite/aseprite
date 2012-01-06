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

#include <allegro.h>

#include "app.h"
#include "base/exception.h"
#include "base/memory.h"
#include "base/memory_dump.h"
#include "console.h"
#include "loadpng.h"
#include "resource_finder.h"

#ifdef WIN32
#include <winalleg.h>
#endif

//////////////////////////////////////////////////////////////////////
// Information for "ident".

const char ase_ident[] =
    "$" PACKAGE ": " VERSION " " COPYRIGHT " $\n"
    "$Website: " WEBSITE " $\n";

//////////////////////////////////////////////////////////////////////
// Memory leak detector wrapper

#ifdef MEMLEAK
class MemLeak
{
public:
  MemLeak() { base_memleak_init(); }
  ~MemLeak() { base_memleak_exit(); }
};
#endif

//////////////////////////////////////////////////////////////////////
// Allegro libray initialization

class Allegro {
public:
  Allegro() {
    allegro_init();
    set_uformat(U_ASCII);
    install_timer();

    // Register PNG as a supported bitmap type
    register_bitmap_file_type("png", load_png, save_png);
  }
  ~Allegro() {
    remove_timer();
    allegro_exit();
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
#ifdef WIN32
  CoInitialize(NULL);
#endif

  base::MemoryDump memoryDump;
  Allegro allegro;
#ifdef MEMLEAK
  MemLeak memleak;
#endif
  Jinete jinete;
  App app(argc, argv);

  // Change the name of the memory dump file
  {
    std::string filename;
    if (get_memory_dump_filename(filename))
      memoryDump.setFileName(filename);
  }

  return app.run();
}

END_OF_MAIN();
