// Aseprite Document Library
// Copyright (c) 2020  Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/serialization.h"
#include "doc/palette.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>

namespace doc {
namespace file {

using namespace base::serialization::big_endian;

const int ActMaxColors = 256;

Palette* load_act_file(const char *filename)
{
  std::ifstream f(FSTREAM_PATH(filename), std::ios::binary);
  if (f.bad())
    return nullptr;

  // Each color is a 24-bit RGB value
  uint8_t rgb[ActMaxColors * 3] = { 0 };
  f.read(reinterpret_cast<char*>(&rgb), sizeof(rgb));

  int colors = ActMaxColors;
  // If there's extra bytes, it's the number of colors to use
  if (!f.eof()) {
    colors = std::min(read16(f), (uint16_t)ActMaxColors);
  }

  std::unique_ptr<Palette> pal(new Palette(frame_t(0), colors));

  uint8_t *c = rgb;
  for (int i = 0; i < colors; ++i) {
    uint8_t r = *(c++);
    uint8_t g = *(c++);
    uint8_t b = *(c++);

    pal->setEntry(i, rgba(r, g, b, 255));
  }

  return pal.release();
}

bool save_act_file(const Palette *pal, const char *filename)
{
  std::ofstream f(FSTREAM_PATH(filename), std::ios::binary);
  if (f.bad())
    return false;

  // Need to write 256 colors even if the palette is smaller
  uint8_t rgb[ActMaxColors * 3] = { 0 };
  uint8_t *c = rgb;

  int colors = std::min(pal->size(), ActMaxColors);
  for (int i = 0; i < colors; ++i) {
    uint32_t col = pal->getEntry(i);

    *(c++) = rgba_getr(col);
    *(c++) = rgba_getg(col);
    *(c++) = rgba_getb(col);
  }
  f.write(reinterpret_cast<char*>(&rgb), sizeof(rgb));

  write16(f, colors);
  write16(f, 0);

  return true;
}

} // namespace file
} // namespace doc
