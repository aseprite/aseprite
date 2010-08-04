/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "raster/cel.h"
#include "raster/layer.h"

//////////////////////////////////////////////////////////////////////

Cel::Cel(int frame, int image)
  : GfxObj(GFXOBJ_CEL)
{
  this->frame = frame;
  this->image = image;
  this->x = 0;
  this->y = 0;
  this->opacity = 255;
}

Cel::Cel(const Cel& cel)
  : GfxObj(cel)
{
  this->frame = cel.frame;
  this->image = cel.image;
  this->x = cel.x;
  this->y = cel.y;
  this->opacity = cel.opacity;
}

Cel::~Cel()
{
}

//////////////////////////////////////////////////////////////////////

Cel* cel_new(int frame, int image)
{
  return new Cel(frame, image);
}

Cel* cel_new_copy(const Cel* cel)
{
  ASSERT(cel);
  return new Cel(*cel);
}

void cel_free(Cel* cel)
{
  ASSERT(cel);
  delete cel;
}

/**
 * @warning You have to remove the cel from the layer before to change
 *          the frame position.
 */
void cel_set_frame(Cel* cel, int frame)
{
  cel->frame = frame;
}

void cel_set_image(Cel* cel, int image)
{
  cel->image = image;
}

void cel_set_position(Cel* cel, int x, int y)
{
  cel->x = x;
  cel->y = y;
}

void cel_set_opacity(Cel* cel, int opacity)
{
  cel->opacity = opacity;
}

