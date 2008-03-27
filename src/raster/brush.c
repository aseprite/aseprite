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

#include <allegro/base.h>
#include <math.h>

#include "jinete/jbase.h"

#include "raster/brush.h"
#include "raster/image.h"

static void clean_brush (Brush *brush);
static void regenerate_brush (Brush *brush);

Brush *brush_new (void)
{
  Brush *brush;

  brush = jnew (Brush, 1);
  if (!brush)
    return NULL;

  brush->type = BRUSH_CIRCLE;
  brush->size = 1;
  brush->angle = 0;
  brush->image = NULL;
  brush->scanline = NULL;

  regenerate_brush (brush);

  return brush;
}

/* Makes a copy of "brush". */
Brush *brush_new_copy (const Brush *brush)
{
  Brush *brush_copy;

  brush_copy = brush_new ();
  if (!brush_copy)
    return NULL;

  brush_copy->type = brush->type;
  brush_copy->size = brush->size;
  brush_copy->angle = brush->angle;
  regenerate_brush (brush_copy);

  return brush_copy;
}

void brush_free (Brush *brush)
{
  clean_brush (brush);
  jfree (brush);
}

void brush_set_type (Brush *brush, int type)
{
  brush->type = type;
  regenerate_brush (brush);
}

void brush_set_size (Brush *brush, int size)
{
  brush->size = size;
  regenerate_brush (brush);
}

void brush_set_angle (Brush *brush, int angle)
{
  brush->angle = angle;
  regenerate_brush (brush);
}

/* Cleans the brush's data (image and region). */
static void clean_brush (Brush *brush)
{
  if (brush->image) {
    image_free (brush->image);
    brush->image = NULL;
  }

  if (brush->scanline) {
    jfree (brush->scanline);
    brush->scanline = NULL;
  }
}

/* Regenerates the brush bitmap and its rectangle's region. */
static void regenerate_brush(Brush *brush)
{
  int x, y;

  clean_brush(brush);

  brush->image = image_new(IMAGE_BITMAP, brush->size, brush->size);
  image_clear(brush->image, 0);

  switch (brush->type) {

    case BRUSH_CIRCLE:
      image_ellipsefill(brush->image, 0, 0, brush->size-1, brush->size-1, 1);
      break;

    case BRUSH_SQUARE:
      image_rectfill(brush->image, 0, 0, brush->size-1, brush->size-1, 1);
      break;

    case BRUSH_LINE: {
      double a = PI * brush->angle / 180;
      int r = brush->size/2;

      x = cos(a) * r;
      y = -sin(a) * r;
      image_line(brush->image, r-x, r-y, r+x, r+y, 1);
      image_line(brush->image, r-x-1, r-y, r+x-1, r+y, 1);
      break;
    }
  }

  brush->scanline = jnew(struct BrushScanline, brush->size);
  for (y=0; y<brush->size; y++) {
    brush->scanline[y].state = FALSE;

    for (x=0; x<brush->size; x++) {
      if (image_getpixel(brush->image, x, y)) {
	brush->scanline[y].x1 = x;

	for (; x<brush->size; x++)
	  if (!image_getpixel(brush->image, x, y))
	    break;

	brush->scanline[y].x2 = x-1;
	brush->scanline[y].state = TRUE;
	break;
      }
    }
  }
}

