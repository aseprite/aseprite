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

#ifndef RASTER_MASK_H_INCLUDED
#define RASTER_MASK_H_INCLUDED

#include "raster/gfxobj.h"
#include "raster/image.h"

// Represents the selection (selected pixels, 0/1, 0=non-selected, 1=selected)
class Mask : public GfxObj
{
  int m_freeze_count;
public:
  // TODO private this
  char* name;			/* mask name */
  int x, y, w, h;		/* region bounds */
  Image* bitmap;		/* bitmapped image mask */

  Mask();
  Mask(const Mask& mask);
  virtual ~Mask();

  // Returns true if the mask is completely empty (i.e. nothing
  // selected)
  bool is_empty() const {
    return (!this->bitmap) ? true: false;
  }

  // Returns true if the point is inside the mask
  bool contains_point(int u, int v) const {
    return (bitmap &&
	    u >= x && u < x+w &&
	    v >= y && v < y+h &&
	    image_getpixel(bitmap, u-x, v-y));
  }

  // These functions can be used to disable the automatic call to
  // "shrink" method (so you can do a lot of modifications without
  // lossing time shrink the mask in each little operation)
  void freeze();
  void unfreeze();

  // Adds the specified rectangle in the mask/selection
  void add(int x, int y, int w, int h);

  // Reserve a rectangle to draw onto the bitmap (you should call
  // shrink after you draw in the bitmap)
  void reserve(int x, int y, int w, int h);

  // Shrinks all sides of the mask to the minimum possible looking at
  // empty pixels in the bitmap
  void shrink();

private:
  void initialize();
};

Mask* mask_new();
Mask* mask_new_copy(const Mask* mask);
void mask_free(Mask* mask);

void mask_set_name(Mask* mask, const char *name);

void mask_copy(Mask* mask_dst, const Mask* mask_src);
void mask_move(Mask* mask, int x, int y);
void mask_none(Mask* mask);
void mask_invert(Mask* mask);
void mask_replace(Mask* mask, int x, int y, int w, int h);
void mask_union(Mask* mask, int x, int y, int w, int h);
void mask_subtract(Mask* mask, int x, int y, int w, int h);
void mask_intersect(Mask* mask, int x, int y, int w, int h);

void mask_by_color(Mask* mask, const Image* image, int color, int fuzziness);
void mask_crop(Mask* mask, const Image* image);

#endif
