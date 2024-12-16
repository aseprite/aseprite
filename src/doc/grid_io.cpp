// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/grid_io.h"

#include "base/serialization.h"
#include "doc/grid.h"
#include "doc/image.h"
#include "doc/image_io.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

bool write_grid(std::ostream& os, const Grid& grid)
{
  write32(os, grid.tileSize().w);
  write32(os, grid.tileSize().h);
  write32(os, grid.origin().x);
  write32(os, grid.origin().y);
  write32(os, grid.tileCenter().x);
  write32(os, grid.tileCenter().y);
  write32(os, grid.tileOffset().x);
  write32(os, grid.tileOffset().y);
  write32(os, grid.oddRowOffset().x);
  write32(os, grid.oddRowOffset().y);
  write32(os, grid.oddColOffset().x);
  write32(os, grid.oddColOffset().y);
  write8(os, grid.hasMask());
  if (grid.hasMask())
    return write_image(os, grid.mask().get());
  else
    return true;
}

Grid read_grid(std::istream& is)
{
  gfx::Size tileSize;
  gfx::Point origin;
  gfx::Point tileCenter;
  gfx::Point tileOffset;
  gfx::Point oddRowOffset;
  gfx::Point oddColOffset;
  tileSize.w = read32(is);
  tileSize.h = read32(is);
  origin.x = read32(is);
  origin.y = read32(is);
  tileCenter.x = read32(is);
  tileCenter.y = read32(is);
  tileOffset.x = read32(is);
  tileOffset.y = read32(is);
  oddRowOffset.x = read32(is);
  oddRowOffset.y = read32(is);
  oddColOffset.x = read32(is);
  oddColOffset.y = read32(is);
  bool hasMask = read8(is);

  Grid grid;
  grid.tileSize(tileSize);
  grid.origin(origin);
  grid.tileCenter(tileCenter);
  grid.tileOffset(tileOffset);
  grid.oddRowOffset(oddRowOffset);
  grid.oddColOffset(oddColOffset);

  if (hasMask) {
    ImageRef mask(read_image(is));
    if (mask)
      grid.setMask(mask);
  }
  return grid;
}

} // namespace doc
