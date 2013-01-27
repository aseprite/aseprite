/* ASEPRITE
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

#include "config.h"

#include "raster/cel_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/cel.h"

#include <iostream>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Cel data:
//
//   WORD               Frame
//   WORD               Image index
//   WORD[2]            X, Y
//   WORD               Opacity

void write_cel(std::ostream& os, Cel* cel)
{
  // ObjectId cel_id = objects->addObject(cel);
  // write_raw_uint32(cel_id);

  write16(os, cel->getFrame());
  write16(os, cel->getImage());
  write16(os, (int16_t)cel->getX());
  write16(os, (int16_t)cel->getY());
  write16(os, cel->getOpacity());

  // objects->removeObject(cel_id);
}

Cel* read_cel(std::istream& is)
{
  // ObjectId cel_id = read32();

  FrameNumber frame(read16(is));
  int imageIndex = read16(is);
  int x = (int16_t)read16(is);
  int y = (int16_t)read16(is);
  int opacity = read16(is);

  UniquePtr<Cel> cel(new Cel(frame, imageIndex));

  cel->setPosition(x, y);
  cel->setOpacity(opacity);

  // objects->insertObject(cel_id, cel);
  return cel.release();
}

}
