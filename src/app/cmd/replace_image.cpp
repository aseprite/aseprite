/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/cmd/replace_image.h"

#include "app/cmd/object_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/image_ref.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

ReplaceImage::ReplaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage)
  : WithSprite(sprite)
  , m_oldImageId(oldImage->id())
  , m_newImageId(newImage->id())
  , m_newImage(newImage)
{
}

void ReplaceImage::onExecute()
{
  // Save old image in m_copy. We cannot keep an ImageRef to this
  // image, because there are other undo branches that could try to
  // modify/re-add this same image ID
  ImageRef oldImage = sprite()->getImage(m_oldImageId);
  ASSERT(oldImage);
  m_copy.reset(Image::createCopy(oldImage));

  sprite()->replaceImage(m_oldImageId, m_newImage);
  m_newImage.reset(nullptr);
}

void ReplaceImage::onUndo()
{
  ImageRef newImage = sprite()->getImage(m_newImageId);
  ASSERT(newImage);
  ASSERT(!sprite()->getImage(m_oldImageId));
  m_copy->setId(m_oldImageId);

  sprite()->replaceImage(m_newImageId, m_copy);
  m_copy.reset(Image::createCopy(newImage));
}

void ReplaceImage::onRedo()
{
  ImageRef oldImage = sprite()->getImage(m_oldImageId);
  ASSERT(oldImage);
  ASSERT(!sprite()->getImage(m_newImageId));
  m_copy->setId(m_newImageId);

  sprite()->replaceImage(m_oldImageId, m_copy);
  m_copy.reset(Image::createCopy(oldImage));
}

} // namespace cmd
} // namespace app
