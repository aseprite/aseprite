/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/undoers/image_area.h"

#include "raster/image.h"
#include "undo/objects_container.h"
#include "undo/undo_exception.h"
#include "undo/undoers_collector.h"

#include <algorithm>

namespace app {
namespace undoers {

using namespace undo;

ImageArea::ImageArea(ObjectsContainer* objects, Image* image, int x, int y, int w, int h)
  : m_imageId(objects->addObject(image))
  , m_format(image->pixelFormat())
  , m_x(x), m_y(y), m_w(w), m_h(h)
  , m_lineSize(image->getRowStrideSize(w))
  , m_data(m_lineSize * h)
{
  ASSERT(w >= 1 && h >= 1);
  ASSERT(x >= 0 && y >= 0 && x+w <= image->width() && y+h <= image->height());

  std::vector<uint8_t>::iterator it = m_data.begin();
  for (int v=0; v<h; ++v) {
    uint8_t* addr = image->getPixelAddress(x, y+v);
    std::copy(addr, addr+m_lineSize, it);
    it += m_lineSize;
  }
}

void ImageArea::dispose()
{
  delete this;
}

void ImageArea::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);

  if (image->pixelFormat() != m_format)
    throw UndoException("Image type does not match");

  // Backup the current image portion
  redoers->pushUndoer(new ImageArea(objects, image, m_x, m_y, m_w, m_h));

  // Restore the old image portion
  std::vector<uint8_t>::iterator it = m_data.begin();
  for (int v=0; v<m_h; ++v) {
    uint8_t* addr = image->getPixelAddress(m_x, m_y+v);
    std::copy(it, it+m_lineSize, addr);
    it += m_lineSize;
  }
}

} // namespace undoers
} // namespace app
