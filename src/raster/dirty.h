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

#ifndef RASTER_DIRTY_H_INCLUDED
#define RASTER_DIRTY_H_INCLUDED

#include "raster/image.h"

class Pen;
class Image;
class Mask;

#define DIRTY_VALID_COLUMN	1
#define DIRTY_MUSTBE_UPDATED	2

struct DirtyCol
{
  int x, w;
  char flags;
  void* data;
  void* ptr;
};

struct DirtyRow
{
  int y;
  int cols;
  DirtyCol* col;
};

struct Dirty
{
  Image* image;
  int x1, y1;
  int x2, y2;
  bool tiled;
  int rows;
  DirtyRow* row;
  Mask* mask;
};

Dirty* dirty_new(Image* image, int x1, int y1, int x2, int y2, bool tiled);
Dirty* dirty_new_copy(Dirty* src);
Dirty* dirty_new_from_differences(Image* image, Image* image_diff);
void dirty_free(Dirty* dirty);

void dirty_putpixel(Dirty* dirty, int x, int y);
void dirty_hline(Dirty* dirty, int x1, int y, int x2);
void dirty_vline(Dirty* dirty, int x, int y1, int y2);
void dirty_line(Dirty* dirty, int x1, int y1, int x2, int y2);
void dirty_rect(Dirty* dirty, int x1, int y1, int x2, int y2);
void dirty_rectfill(Dirty* dirty, int x1, int y1, int x2, int y2);

void dirty_putpixel_pen(Dirty* dirty, Pen* pen, int x, int y);
void dirty_hline_pen(Dirty* dirty, Pen* pen, int x1, int y, int x2);
void dirty_line_pen(Dirty* dirty, Pen* pen, int x1, int y1, int x2, int y2);

void dirty_save_image_data(Dirty* dirty);
void dirty_restore_image_data(Dirty* dirty);
void dirty_swap(Dirty* dirty);

inline int dirty_line_size(Dirty* dirty, int width)
{
  return image_line_size(dirty->image, width);
}

#endif

