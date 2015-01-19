/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef APP_CMD_REPLACE_IMAGE_H_INCLUDED
#define APP_CMD_REPLACE_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
// #include "app/cmd/with_image.h"
#include "doc/image_ref.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class ReplaceImage : public Cmd
                     , public WithSprite {
  public:
    ReplaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_copy ? m_copy->getMemSize(): 0);
    }

  private:
    // Reference used only to keep the copy of the new image from the
    // ReplaceImage() ctor until the ReplaceImage::onExecute() call.
    // Then the reference is not used anymore.
    ImageRef m_newImage;

    ObjectId m_oldImageId;
    ObjectId m_newImageId;
    ImageRef m_copy;
  };

} // namespace cmd
} // namespace app

#endif
