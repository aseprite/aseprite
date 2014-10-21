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

#include "app/undoers/remove_image.h"

#include "app/undoers/add_image.h"
#include "app/undoers/object_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/stock.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace doc;
using namespace undo;

RemoveImage::RemoveImage(ObjectsContainer* objects, Stock* stock, int imageIndex)
  : m_stockId(objects->addObject(stock))
  , m_imageIndex(imageIndex)
{
  Image* image = stock->getImage(imageIndex);

  write_object(objects, m_stream, image, doc::write_image);
}

void RemoveImage::dispose()
{
  delete this;
}

void RemoveImage::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Stock* stock = objects->getObjectT<Stock>(m_stockId);
  Image* image = read_object<Image>(objects, m_stream, doc::read_image);

  // Push an AddImage as redoer
  redoers->pushUndoer(new AddImage(objects, stock, m_imageIndex));

  stock->replaceImage(m_imageIndex, image);
}

} // namespace undoers
} // namespace app
