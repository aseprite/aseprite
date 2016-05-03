// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/split_string.h"
#include "base/trim_string.h"
#include "base/unique_ptr.h"
#include "doc/image.h"
#include "doc/palette.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace doc {
namespace file {

Palette* load_gpl_file(const char *filename)
{
  std::ifstream f(FSTREAM_PATH(filename));
  if (f.bad()) return NULL;

  // Read first line, it must be "GIMP Palette"
  std::string line;
  if (!std::getline(f, line)) return NULL;
  base::trim_string(line, line);
  if (line != "GIMP Palette") return NULL;

  base::UniquePtr<Palette> pal(new Palette(frame_t(0), 0));

  while (std::getline(f, line)) {
    // Trim line.
    base::trim_string(line, line);

    // Remove comments
    if (line.empty() || line[0] == '#')
      continue;

    // Remove properties (TODO add these properties in the palette)
    if (!std::isdigit(line[0]))
      continue;

    int r, g, b;
    std::istringstream lineIn(line);
    // TODO add support to read the color name
    lineIn >> r >> g >> b;

    if (lineIn.fail())
        continue;

    pal->addEntry(rgba(r, g, b, 255));
  }

  return pal.release();
}

bool save_gpl_file(const Palette *pal, const char *filename)
{
  std::ofstream f(FSTREAM_PATH(filename));
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
} // namespace doc
