/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef RASTER_FRAME_H
#define RASTER_FRAME_H

#include "raster/gfxobj.h"

struct Layer;

typedef struct Frame Frame;

struct Frame
{
  GfxObj gfxobj;
  int frpos;			/* frame position */
  int image;			/* image index of stock */
  int x, y;			/* X/Y screen position */
  int opacity;			/* opacity level */
};

Frame *frame_new (int frpos, int image);
Frame *frame_new_copy (const Frame *frame);
void frame_free (Frame *frame);

Frame *frame_is_link (Frame *frame, struct Layer *layer);

void frame_set_frpos (Frame *frame, int frpos);
void frame_set_image (Frame *frame, int image);
void frame_set_position (Frame *frame, int x, int y);
void frame_set_opacity (Frame *frame, int opacity);

#endif /* RASTER_FRAME_H */
