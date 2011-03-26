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

#include "undoers/flip_image.h"

#include "base/unique_ptr.h"
#include "raster/image.h"
#include "undo/objects_container.h"
#include "undo/undo_exception.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;

FlipImage::FlipImage(ObjectsContainer* objects, Image* image, int x, int y, int w, int h, FlipType flipType)
  : m_imageId(objects->addObject(image))
  , m_imgtype(image->imgtype)
  , m_x(x), m_y(y), m_w(w), m_h(h)
  , m_flipType(flipType)
{
  ASSERT(w >= 1 && h >= 1);
  ASSERT(x >= 0 && y >= 0 && x+w <= image->w && y+h <= image->h);
}

void FlipImage::dispose()
{
  delete this;
}

void FlipImage::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);

  if (image->imgtype != m_imgtype)
    throw UndoException("Image type does not match");

  redoers->pushUndoer(new FlipImage(objects, image, m_x, m_y, m_w, m_h, m_flipType));

  UniquePtr<Image> area(image_crop(image, m_x, m_y, m_w, m_h, 0));
  int x, y;
  int x2 = m_x+m_w-1;
  int y2 = m_y+m_h-1;

  for (y=0; y<m_h; ++y)
    for (x=0; x<m_w; ++x)
      image_putpixel(image,
		     m_flipType == FlipHorizontal ? x2-x: m_x+x,
		     m_flipType == FlipVertical   ? y2-y: m_y+y,
		     image_getpixel(area, x, y));
}
