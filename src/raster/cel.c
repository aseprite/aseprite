/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

Cel *cel_new(int frame, int image)
{
  Cel *cel = (Cel *)gfxobj_new(GFXOBJ_CEL, sizeof(Cel));
  if (!cel)
    return NULL;

  cel->frame = frame;
  cel->image = image;
  cel->x = 0;
  cel->y = 0;
  cel->opacity = 255;

  return cel;
}

Cel *cel_new_copy(const Cel *cel)
{
  Cel *cel_copy;

  cel_copy = cel_new(cel->frame, cel->image);
  if (!cel_copy)
    return NULL;
  cel_set_position(cel_copy, cel->x, cel->y);
  cel_set_opacity(cel_copy, cel->opacity);

  return cel_copy;
}

void cel_free(Cel *cel)
{
  gfxobj_free((GfxObj *)cel);
}

Cel *cel_is_link(Cel *cel, Layer *layer)
{
  Cel *link;
  int frame;

  for (frame=0; frame<cel->frame; frame++) {
    link = layer_get_cel(layer, frame);
    if (link && link->image == cel->image)
      return link;
  }

  return NULL;
}

/**
 * @warning You have to remove the cel from the layer before to change
 *          the frame position.
 */
void cel_set_frame(Cel *cel, int frame)
{
  cel->frame = frame;
}

void cel_set_image(Cel *cel, int image)
{
  cel->image = image;
}

void cel_set_position(Cel *cel, int x, int y)
{
  cel->x = x;
  cel->y = y;
}

void cel_set_opacity(Cel *cel, int opacity)
{
  cel->opacity = opacity;
}

