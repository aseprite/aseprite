/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "undoers/dirty_area.h"

#include "base/unique_ptr.h"
#include "raster/dirty.h"
#include "raster/dirty_io.h"
#include "raster/image.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;

DirtyArea::DirtyArea(ObjectsContainer* objects, Image* image, Dirty* dirty)
  : m_imageId(objects->addObject(image))
{
  raster::write_dirty(m_stream, dirty);
}

void DirtyArea::dispose()
{
  delete this;
}

void DirtyArea::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);
  UniquePtr<Dirty> dirty(raster::read_dirty(m_stream));

  // Swap the saved pixels in the dirty with the pixels in the image
  dirty->swapImagePixels(image);

  // Save the dirty in the "redoers" (the dirty area now contains the
  // pixels before the undo)
  redoers->pushUndoer(new DirtyArea(objects, image, dirty));
}
