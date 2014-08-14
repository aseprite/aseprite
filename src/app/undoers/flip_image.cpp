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

#include "app/undoers/flip_image.h"

#include "base/unique_ptr.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "raster/algorithm/flip_image.h"
#include "raster/image.h"
#include "undo/objects_container.h"
#include "undo/undo_exception.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace raster;
using namespace undo;

FlipImage::FlipImage(ObjectsContainer* objects, Image* image,
                     const gfx::Rect& bounds,
                     raster::algorithm::FlipType flipType)
  : m_imageId(objects->addObject(image))
  , m_format(image->pixelFormat())
  , m_x(bounds.x), m_y(bounds.y)
  , m_w(bounds.w), m_h(bounds.h)
  , m_flipType(flipType)
{
  ASSERT(m_w >= 1 && m_h >= 1);
  ASSERT(m_x >= 0 && m_y >= 0 && m_x+m_w <= image->width() && m_y+m_h <= image->height());
}

void FlipImage::dispose()
{
  delete this;
}

void FlipImage::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Image* image = objects->getObjectT<Image>(m_imageId);
  raster::algorithm::FlipType flipType = static_cast<raster::algorithm::FlipType>(m_flipType);
  gfx::Rect bounds(gfx::Point(m_x, m_y), gfx::Size(m_w, m_h));

  if (image->pixelFormat() != m_format)
    throw UndoException("Image type does not match");

  redoers->pushUndoer(new FlipImage(objects, image, bounds, flipType));

  raster::algorithm::flip_image(image, bounds, flipType);
}

} // namespace undoers
} // namespace app
