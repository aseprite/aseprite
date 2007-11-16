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

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include "raster/frame.h"
#include "raster/layer.h"

#endif

Frame *frame_new (int frpos, int image)
{
  Frame *frame = (Frame *)gfxobj_new (GFXOBJ_FRAME, sizeof(Frame));
  if (!frame)
    return NULL;

  frame->frpos = frpos;
  frame->image = image;
  frame->x = 0;
  frame->y = 0;
  frame->opacity = 255;

  return frame;
}

Frame *frame_new_copy (const Frame *frame)
{
  Frame *frame_copy;

  frame_copy = frame_new (frame->frpos, frame->image);
  if (!frame_copy)
    return NULL;
  frame_set_position (frame_copy, frame->x, frame->y);
  frame_set_opacity (frame_copy, frame->opacity);

  return frame_copy;
}

void frame_free (Frame *frame)
{
  gfxobj_free ((GfxObj *)frame);
}

Frame *frame_is_link (Frame *frame, Layer *layer)
{
  Frame *link;
  int frpos;

  for (frpos=0; frpos<frame->frpos; frpos++) {
    link = layer_get_frame (layer, frpos);
    if (link && link->image == frame->image)
      return link;
  }

  return NULL;
}

/* warning, you must remove the frame from parent layer before to
   change the frpos */
void frame_set_frpos (Frame *frame, int frpos)
{
  frame->frpos = frpos;
}

void frame_set_image (Frame *frame, int image)
{
  frame->image = image;
}

void frame_set_position (Frame *frame, int x, int y)
{
  frame->x = x;
  frame->y = y;
}

void frame_set_opacity (Frame *frame, int opacity)
{
  frame->opacity = opacity;
}

