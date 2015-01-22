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

#include "app/cmd/object_io.h"

#include "doc/cel.h"
#include "doc/cel_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_io.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

ObjectIO::ObjectIO(Sprite* sprite)
  : m_sprite(sprite)
{
}

ObjectIO::~ObjectIO()
{
}

void ObjectIO::write_cel(std::ostream& os, Cel* cel)
{
  write_object(os, cel, [this](std::ostream& os, Cel* cel) {
      doc::write_cel(os, this, cel);
    });
}

void ObjectIO::write_image(std::ostream& os, Image* image)
{
  write_object(os, image, doc::write_image);
}

void ObjectIO::write_layer(std::ostream& os, Layer* layer)
{
  write_object(os, layer, [this](std::ostream& os, Layer* layer) {
      doc::write_layer(os, this, layer);
    });
}

Cel* ObjectIO::read_cel(std::istream& is)
{
  return read_object<Cel>(is, [this](std::istream& is) {
      return doc::read_cel(is, this, m_sprite);
    });
}

Image* ObjectIO::read_image(std::istream& is)
{
  return read_object<Image>(is, doc::read_image);
}

Layer* ObjectIO::read_layer(std::istream& is)
{
  return read_object<Layer>(is, [this](std::istream& is) {
      return doc::read_layer(is, this, m_sprite);
    });
}

void ObjectIO::add_image_ref(const ImageRef& image)
{
  ASSERT(image);
  ASSERT(!get_image_ref(image->id()));
  m_images.push_back(image);
}

ImageRef ObjectIO::get_image_ref(ObjectId imageId)
{
  for (ImageRef& image : m_images) {
    if (image->id() == imageId) {
      ASSERT(!m_sprite->getImage(imageId));
      return image;
    }
  }

  return m_sprite->getImage(imageId);
}

} // namespace cmd
} // namespace app
