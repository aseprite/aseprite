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

#include "app/undoers/replace_image.h"

#include "app/undoers/object_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/image_ref.h"
#include "doc/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;

ReplaceImage::ReplaceImage(ObjectsContainer* objects, Sprite* sprite,
  Image* oldImage, Image* newImage)
  : m_spriteId(objects->addObject(sprite))
  , m_newImageId(objects->addObject(newImage))
{
  ObjectIO(objects, sprite).write_image(m_stream, oldImage);
}

void ReplaceImage::dispose()
{
  delete this;
}

void ReplaceImage::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);
  Image* currentImageRaw = objects->getObjectT<Image>(m_newImageId);
  ASSERT(currentImageRaw != NULL);

  ImageRef currentImage = sprite->getImage(currentImageRaw->id());
  ASSERT(currentImage != NULL);

  // Read the image to be restored from the stream
  ImageRef restoreImage(
    ObjectIO(objects, sprite).read_image(m_stream));

  // Save the current image in the redoers
  redoers->pushUndoer(new ReplaceImage(objects, sprite,
      currentImage, restoreImage));

  // Replace the image in the stock
  sprite->replaceImage(currentImage->id(), restoreImage);
}

} // namespace undoers
} // namespace app
