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

#ifndef RASTER_MASK_H
#define RASTER_MASK_H

#include "raster/gfxobj.h"

struct Image;

typedef struct Mask Mask;

struct Mask
{
  GfxObj gfxobj;
  char *name;			/* mask name */
  int x, y, w, h;		/* region bounds */
  struct Image *bitmap;		/* bitmapped image mask */
};

Mask *mask_new (void);
Mask *mask_new_copy (const Mask *mask);
void mask_free (Mask *mask);

int mask_is_empty (Mask *mask);
void mask_set_name (Mask *mask, const char *name);

void mask_copy (Mask *mask_dst, const Mask *mask_src);
void mask_move (Mask *mask, int x, int y);
void mask_none (Mask *mask);
void mask_invert (Mask *mask);
void mask_replace (Mask *mask, int x, int y, int w, int h);
void mask_union (Mask *mask, int x, int y, int w, int h);
void mask_subtract (Mask *mask, int x, int y, int w, int h);
void mask_intersect (Mask *mask, int x, int y, int w, int h);

void mask_merge (Mask *dst, const Mask *src);
void mask_by_color (Mask *mask, const struct Image *image, int color, int fuzziness);
void mask_crop (Mask *mask, const struct Image *image);

#endif /* RASTER_MASK_H */
