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

#include "raster/frame_number.h"
#include "raster/object.h"

namespace raster {

  class LayerImage;

  class Cel : public Object {
  public:
    Cel(FrameNumber frame, int image);
    Cel(const Cel& cel);
    virtual ~Cel();

    FrameNumber getFrame() const { return m_frame; }
    int getImage() const { return m_image; }
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    int getOpacity() const { return m_opacity; }

    void setFrame(FrameNumber frame) { m_frame = frame; }
    void setImage(int image) { m_image = image; }
    void setPosition(int x, int y) { m_x = x; m_y = y; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    int getMemSize() const {
      return sizeof(Cel);
    }

  private:
    FrameNumber m_frame;          // Frame position
    int m_image;                  // Image index of stock
    int m_x, m_y;                 // X/Y screen position
    int m_opacity;                // Opacity level
  };

} // namespace raster

#endif
