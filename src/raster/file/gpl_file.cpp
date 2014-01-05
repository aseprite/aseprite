/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "base/split_string.h"
#include "base/trim_string.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/palette.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

namespace raster {
namespace file {

Palette* load_gpl_file(const char *filename)
{
  std::ifstream f(filename);
  if (f.bad()) return NULL;

  // Read first line, it must be "GIMP Palette"
  std::string line;
  if (!std::getline(f, line)) return NULL;
  base::trim_string(line, line);
  if (line != "GIMP Palette") return NULL;

  base::UniquePtr<Palette> pal(new Palette(FrameNumber(0), 256));
  int entryCounter = 0;

  while (std::getline(f, line)) {
    // Trim line.
    base::trim_string(line, line);

    // Remove comments
    if (line.empty() || line[0] == '#')
      continue;

    int r, g, b;
    std::istringstream lineIn(line);
    lineIn >> r >> g >> b;
    if (lineIn.good()) {
      pal->setEntry(entryCounter, rgba(r, g, b, 255));
      ++entryCounter;
      if (entryCounter >= Palette::MaxColors)
        break;
    }
  }

  return pal.release();
}

bool save_gpl_file(const Palette *pal, const char *filename)
{
  std::ofstream f(filename);
  if (f.bad()) return false;

  f << "GIMP Palette\n"
    << "#\n";

  for (int i=0; i<pal->size(); ++i) {
    uint32_t col = pal->getEntry(i);
    f << std::setfill(' ') << std::setw(3) << ((int)rgba_getr(col)) << " "
      << std::setfill(' ') << std::setw(3) << ((int)rgba_getg(col)) << " "
      << std::setfill(' ') << std::setw(3) << ((int)rgba_getb(col)) << "\tUntitled\n";
  }

  return true;
}

} // namespace file
} // namespace raster
