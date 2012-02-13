/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "undoers/image_area.h"

#include "raster/image.h"
#include "undo/objects_container.h"
#include "undo/undo_exception.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;

ImageArea::ImageArea(ObjectsContainer* objects, Image* image, int x, int y, int w, int h)
  : m_imageId(objects->addObject(image))
  , m_format(image->getPixelFormat())
  , m_x(x), m_y(y), m_w(w), m_h(h)
  , m_lineSize(image_line_size(image, w))
  , m_data(m_lineSize * h)
{
  ASSERT(w >= 1 && h >= 1);
  ASSERT(x >= 0 && y >= 0 && x+w <= image->w && y+h <= image->h);

  for (int v=0; v<h; ++v)
    memcpy(&m_data[m_lineSize*v], image_address(image, x, y+v), m_lineSize);
}

void ImageArea::dispose()
{
  delete this;
}

void ImageArea::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);

  if (image->getPixelFormat() != m_format)
    throw UndoException("Image type does not match");

  // Backup the current image portion
  redoers->pushUndoer(new ImageArea(objects, image, m_x, m_y, m_w, m_h));

  // Restore the old image portion
  for (int v=0; v<m_h; ++v)
    memcpy(image_address(image, m_x, m_y+v), &m_data[m_lineSize*v], m_lineSize);
}
