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

#ifndef RASTER_CEL_H_INCLUDED
#define RASTER_CEL_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "raster/frame_number.h"
#include "raster/object.h"

namespace raster {

  class Image;
  class LayerImage;
  class Sprite;

  class Cel : public Object {
  public:
    Cel(FrameNumber frame, int image);
    Cel(const Cel& cel);
    virtual ~Cel();

    FrameNumber frame() const { return m_frame; }
    int imageIndex() const { return m_image; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    int opacity() const { return m_opacity; }

    LayerImage* layer() const { return m_layer; }
    Image* image() const;
    Sprite* sprite() const;
    gfx::Rect bounds() const;

    // You should change the frame only if the cel isn't member of a
    // layer.  If the cel is already in a layer, you should use
    // LayerImage::moveCel() member function.
    void setFrame(FrameNumber frame) { m_frame = frame; }
    void setImage(int image) { m_image = image; }
    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    int getMemSize() const {
      return sizeof(Cel);
    }

    void setParentLayer(LayerImage* layer) {
      m_layer = layer;
    }

  private:
    LayerImage* m_layer;
    FrameNumber m_frame;          // Frame position
    int m_image;                  // Image index of stock
    int m_x, m_y;                 // X/Y screen position
    int m_opacity;                // Opacity level
  };

} // namespace raster

#endif
