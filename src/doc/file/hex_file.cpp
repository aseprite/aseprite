// Aseprite Document Library
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/hex.h"
#include "base/trim_string.h"
#include "doc/palette.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

namespace doc {
namespace file {

Palette* load_hex_file(const char *filename)
{
  std::ifstream f(FSTREAM_PATH(filename));
  if (f.bad())
    return nullptr;

  std::unique_ptr<Palette> pal(new Palette(frame_t(0), 0));

  // Read line by line, each line one color, ignore everything that
  // doesn't look like a hex color.
  std::string line;
  while (std::getline(f, line)) {
    // Trim line
    base::trim_string(line, line);

    // Remove comments
    if (line.empty())
      continue;

    // Find 6 consecutive hex digits
    for (std::string::size_type i=0; i != line.size(); ++i) {
      std::string::size_type j = i;
      for (; j<i+6; ++j) {
        if (!base::is_hex_digit(line[j]))
          break;
      }
      if (j-i != 6)
        continue;

      // Convert text (Base 16) to integer
      int hex = std::strtol(line.substr(i, 6).c_str(), nullptr, 16);
      int r = (hex & 0xff0000) >> 16;
      int g = (hex & 0xff00) >> 8;
      int b = (hex & 0xff);
      pal->addEntry(rgba(r, g, b, 255));

      // Done, one color per line
      break;
    }
  }

  return pal.release();
}

bool save_hex_file(const Palette *pal, const char *filename)
{
  std::ofstream f(FSTREAM_PATH(filename));
  if (f.bad()) return false;

  f << std::hex << std::setfill('0');
  for (int i=0; i<pal->size(); ++i) {
    uint32_t col = pal->getEntry(i);
    f << std::setw(2) << ((int)rgba_getr(col))
      << std::setw(2) << ((int)rgba_getg(col))
      << std::setw(2) << ((int)rgba_getb(col)) << "\n";
  }

  return true;
}

} // namespace file
} // namespace doc
