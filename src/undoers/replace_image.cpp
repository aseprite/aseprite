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

#include "undoers/replace_image.h"

#include "raster/image.h"
#include "raster/image_io.h"
#include "raster/stock.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"
#include "undoers/object_io.h"

using namespace undo;
using namespace undoers;

ReplaceImage::ReplaceImage(ObjectsContainer* objects, Stock* stock, int imageIndex)
  : m_stockId(objects->addObject(stock))
  , m_imageIndex(imageIndex)
{
  Image* image = stock->getImage(imageIndex);

  write_object(objects, m_stream, image, raster::write_image);
}

void ReplaceImage::dispose()
{
  delete this;
}

void ReplaceImage::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Stock* stock = objects->getObjectT<Stock>(m_stockId);

  // Read the image to be restored from the stream
  Image* image = read_object<Image>(objects, m_stream, raster::read_image);

  // Save the current image in the redoers
  redoers->pushUndoer(new ReplaceImage(objects, stock, m_imageIndex));
  Image* oldImage = stock->getImage(m_imageIndex);

  // Replace the image in the stock
  stock->replaceImage(m_imageIndex, image);

  // Destroy the old image
  delete oldImage;
}
