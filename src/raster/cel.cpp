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

#include "raster/cel.h"

namespace raster {

Cel::Cel(FrameNumber frame, int image)
  : Object(OBJECT_CEL)
  , m_frame(frame)
  , m_image(image)
{
  m_x = 0;
  m_y = 0;
  m_opacity = 255;
}

Cel::Cel(const Cel& cel)
  : Object(cel)
  , m_frame(cel.m_frame)
  , m_image(cel.m_image)
{
  m_x = cel.m_x;
  m_y = cel.m_y;
  m_opacity = cel.m_opacity;
}

Cel::~Cel()
{
}

} // namespace raster
