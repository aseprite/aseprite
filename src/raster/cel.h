/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef RASTER_CEL_H
#define RASTER_CEL_H

#include "raster/gfxobj.h"

struct Layer;

typedef struct Cel Cel;

struct Cel
{
  GfxObj gfxobj;
  int frame;			/* frame position */
  int image;			/* image index of stock */
  int x, y;			/* X/Y screen position */
  int opacity;			/* opacity level */
};

Cel *cel_new(int frame, int image);
Cel *cel_new_copy(const Cel *cel);
void cel_free(Cel *cel);

Cel *cel_is_link(Cel *cel, struct Layer *layer);

void cel_set_frame(Cel *cel, int frame);
void cel_set_image(Cel *cel, int image);
void cel_set_position(Cel *cel, int x, int y);
void cel_set_opacity(Cel *cel, int opacity);

#endif /* RASTER_FRAME_H */
