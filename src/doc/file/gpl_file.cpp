// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/log.h"
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

Palette* load_gpl_file(const char *filename)
{
  std::ifstream f(FSTREAM_PATH(filename));
  if (f.bad()) return NULL;

  // Read first line, it must be "GIMP Palette"
  std::string line;
  if (!std::getline(f, line)) return NULL;
  base::trim_string(line, line);
  if (line != "GIMP Palette") return NULL;

  std::unique_ptr<Palette> pal(new Palette(frame_t(0), 0));
  std::string comment;
  bool hasAlpha = false;

  while (std::getline(f, line)) {
    // Trim line.
    base::trim_string(line, line);

    // Remove empty lines
    if (line.empty())
      continue;

    // Concatenate comments
    if (line[0] == '#') {
      line = line.substr(1);
      base::trim_string(line, line);
      comment += line;
      comment.push_back('\n');
      continue;
    }

    // Remove properties (TODO add these properties in the palette)
    if (!std::isdigit(line[0])) {
      std::vector<std::string> parts;
      base::split_string(line, parts, ":");
      // Aseprite extension for palettes with alpha channel.
      if (parts.size() == 2 &&
          parts[0] == "Channels") {
        base::trim_string(parts[1], parts[1]);
        if (parts[1] == "RGBA")
          hasAlpha = true;
      }
      continue;
    }

    int r, g, b, a = 255;
    std::string entryName;
    std::istringstream lineIn(line);
    lineIn >> r >> g >> b;
    if (hasAlpha) {
      lineIn >> a;
    }
    lineIn >> entryName;

    if (lineIn.fail())
        continue;

    pal->addEntry(rgba(r, g, b, a));
    if (!entryName.empty()) {
      base::trim_string(entryName, entryName);
      if (!entryName.empty())
        pal->setEntryName(pal->size()-1, entryName);
    }
  }

  base::trim_string(comment, comment);
  if (!comment.empty()) {
    LOG(VERBOSE) << "PAL: " << filename << " comment: " << comment << "\n";
    pal->setComment(comment);
  }

  return pal.release();
}

bool save_gpl_file(const Palette* pal, const char* filename)
{
  std::ofstream f(FSTREAM_PATH(filename));
  if (f.bad()) return false;

  const bool hasAlpha = pal->hasAlpha();

  f << "GIMP Palette\n";
  if (hasAlpha)
    f << "Channels: RGBA\n";
  f << "#\n";

  for (int i=0; i<pal->size(); ++i) {
    uint32_t col = pal->getEntry(i);
    f << std::setfill(' ') << std::setw(3) << ((int)rgba_getr(col)) << " "
      << std::setfill(' ') << std::setw(3) << ((int)rgba_getg(col)) << " "
      << std::setfill(' ') << std::setw(3) << ((int)rgba_getb(col));
    if (hasAlpha)
      f << " " << std::setfill(' ') << std::setw(3) << ((int)rgba_geta(col));
    f << "\tUntitled\n";        // TODO add support for color name entries
  }

  return true;
}

} // namespace file
} // namespace doc
