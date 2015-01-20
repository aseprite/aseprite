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

#include "app/cmd/set_cel_image.h"

#include "app/cmd/object_io.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/image_ref.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelImage::SetCelImage(Cel* cel, const ImageRef& newImage)
  : WithCel(cel)
  , m_oldImageId(cel->image()->id())
  , m_newImageId(newImage->id())
  , m_newImage(newImage)
{
}

void SetCelImage::onExecute()
{
  Cel* cel = this->cel();

  if (!cel->links()) {
    ImageRef oldImage = cel->imageRef();
    m_copy.reset(Image::createCopy(oldImage));
  }

  cel->setImage(m_newImage);
  m_newImage.reset(nullptr);
}

void SetCelImage::onUndo()
{
  Cel* cel = this->cel();

  if (m_copy) {
    ASSERT(!cel->sprite()->getImage(m_oldImageId));
    m_copy->setId(m_oldImageId);
    cel->setImage(m_copy);
    m_copy.reset(nullptr);
  }
  else {
    ImageRef oldImage = cel->sprite()->getImage(m_oldImageId);
    ASSERT(oldImage);
    cel->setImage(oldImage);
  }
}

void SetCelImage::onRedo()
{
  Cel* cel = this->cel();

  if (!cel->links()) {
    ImageRef oldImage = cel->imageRef();
    m_copy.reset(Image::createCopy(oldImage));
  }

  ImageRef newImage = cel->sprite()->getImage(m_newImageId);
  ASSERT(newImage);
  cel->setImage(newImage);
  m_newImage.reset(nullptr);
}

} // namespace cmd
} // namespace app
