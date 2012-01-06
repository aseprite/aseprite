/* ASE - Allegro Sprite Editor
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

#include "raster/palette_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/palette.h"

#include <iostream>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Palette data:
//
//   WORD               Frame
//   WORD               Number of colors
//   for each color     ("ncolors" times)
//     DWORD            _rgba color

void write_palette(std::ostream& os, Palette* palette)
{
  write16(os, palette->getFrame()); // Frame
  write16(os, palette->size());     // Number of colors

  for (int c=0; c<palette->size(); c++) {
    uint32_t color = palette->getEntry(c);
    write32(os, color);
  }
}

Palette* read_palette(std::istream& is)
{
  int frame = read16(is);       // Frame
  int ncolors = read16(is);     // Number of colors

  UniquePtr<Palette> palette(new Palette(frame, ncolors));

  for (int c=0; c<ncolors; ++c) {
    uint32_t color = read32(is);
    palette->setEntry(c, color);
  }

  return palette.release();
}

}
