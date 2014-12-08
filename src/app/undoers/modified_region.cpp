/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/undoers/modified_region.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/image.h"
#include "gfx/region.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;
using namespace base::serialization::little_endian;

ModifiedRegion::ModifiedRegion(ObjectsContainer* objects, Image* image, Region& rgn)
  : m_imageId(objects->addObject(image))
{
  // Save region structure
  write32(m_stream, rgn.size());
  for (const auto& rc : rgn) {
    write32(m_stream, rc.x);
    write32(m_stream, rc.y);
    write32(m_stream, rc.w);
    write32(m_stream, rc.h);
  }

  // Save region pixels
  for (const auto& rc : rgn)
    for (int y=0; y<rc.h; ++y)
      m_stream.write(
        (const char*)image->getPixelAddress(rc.x, rc.y+y),
        image->getRowStrideSize(rc.w));
}

void ModifiedRegion::dispose()
{
  delete this;
}

void ModifiedRegion::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);

  gfx::Region rgn;
  int nrects = read32(m_stream);
  for (int n=0; n<nrects; ++n) {
    gfx::Rect rc;
    rc.x = read32(m_stream);
    rc.y = read32(m_stream);
    rc.w = read32(m_stream);
    rc.h = read32(m_stream);
    rgn.createUnion(rgn, gfx::Region(rc));
  }

  // Save the modified region in the "redoers" (the region now
  // contains the pixels before the undo)
  redoers->pushUndoer(new ModifiedRegion(objects, image, rgn));

  // Restore region pixels
  for (const auto& rc : rgn)
    for (int y=0; y<rc.h; ++y)
      m_stream.read(
        (char*)image->getPixelAddress(rc.x, rc.y+y),
        image->getRowStrideSize(rc.w));
}

} // namespace undoers
} // namespace app
