// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/split_string.h"
#include "base/trim_string.h"
#include "doc/image.h"
#include "doc/palette.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

namespace doc {
namespace file {

Palette* load_pal_file(const char *filename)
{
  std::ifstream f(FSTREAM_PATH(filename));
  if (f.bad())
    return nullptr;

  // Read first line, it must be "JASC-PAL"
  std::string line;
  if (!std::getline(f, line))
    return nullptr;
  base::trim_string(line, line);
  if (line != "JASC-PAL")
    return nullptr;

  // Second line is the version (0100)
  if (!std::getline(f, line))
    return nullptr;
  base::trim_string(line, line);
  if (line != "0100")
    return nullptr;

  // Ignore number of colors (we'll read line by line anyway)
  if (!std::getline(f, line))
    return nullptr;

  std::unique_ptr<Palette> pal(new Palette(frame_t(0), 0));

  while (std::getline(f, line)) {
    // Trim line
    base::trim_string(line, line);

    // Remove comments
    if (line.empty())
      continue;

    int r, g, b;
    std::istringstream lineIn(line);
    lineIn >> r >> g >> b;
    pal->addEntry(rgba(r, g, b, 255));
  }

  return pal.release();
}

bool save_pal_file(const Palette *pal, const char *filename)
{
  std::ofstream f(FSTREAM_PATH(filename));
  if (f.bad()) return false;

  f << "JASC-PAL\n"
    << "0100\n"
    << pal->size() << "\n";

  for (int i=0; i<pal->size(); ++i) {
    uint32_t col = pal->getEntry(i);
    f << ((int)rgba_getr(col)) << " "
      << ((int)rgba_getg(col)) << " "
      << ((int)rgba_getb(col)) << "\n";
  }

  return true;
}

} // namespace file
} // namespace doc
