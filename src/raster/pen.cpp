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

#include <allegro/base.h>
#include <math.h>

#include "jinete/jbase.h"

#include "raster/algo.h"
#include "raster/pen.h"
#include "raster/image.h"

Pen::Pen()
{
  m_type = PEN_TYPE_CIRCLE;
  m_size = 1;
  m_angle = 0;
  m_image = NULL;
  m_scanline = NULL;

  regenerate_pen();
}

Pen::Pen(PenType type, int size, int angle)
{
  m_type = type;
  m_size = size;
  m_angle = angle;
  m_image = NULL;
  m_scanline = NULL;

  regenerate_pen();
}

Pen::Pen(const Pen& pen)
{
  m_type = pen.m_type;
  m_size = pen.m_size;
  m_angle = pen.m_angle;

  regenerate_pen();
}

Pen::~Pen()
{
  clean_pen();
}

void Pen::set_type(PenType type)
{
  m_type = type;
  regenerate_pen();
}

void Pen::set_size(int size)
{
  m_size = size;
  regenerate_pen();
}

void Pen::set_angle(int angle)
{
  m_angle = angle;
  regenerate_pen();
}

// Cleans the pen's data (image and region).
void Pen::clean_pen()
{
  if (m_image) {
    image_free(m_image);
    m_image = NULL;
  }

  if (m_scanline) {
    jfree(m_scanline);
    m_scanline = NULL;
  }
}

static void algo_hline(int x1, int y, int x2, void *data)
{
  image_hline(reinterpret_cast<Image*>(data), x1, y, x2, 1);
}

// Regenerates the pen bitmap and its rectangle's region.
void Pen::regenerate_pen()
{
  int x, y;

  clean_pen();

  m_image = image_new(IMAGE_BITMAP, m_size, m_size);
  image_clear(m_image, 0);

  switch (m_type) {

    case PEN_TYPE_CIRCLE:
      image_ellipsefill(m_image, 0, 0, m_size-1, m_size-1, 1);
      break;

    case PEN_TYPE_SQUARE: {
      double a = PI * m_angle / 180;
      int r = m_size/2;
      int x1, y1, x2, y2, x3, y3, x4, y4;

      x1 =  cos(a+  PI/4) * r;
      y1 = -sin(a+  PI/4) * r;
      x2 =  cos(a+3*PI/4) * r;
      y2 = -sin(a+3*PI/4) * r;
      x3 =  cos(a-3*PI/4) * r;
      y3 = -sin(a-3*PI/4) * r;
      x4 =  cos(a-  PI/4) * r;
      y4 = -sin(a-  PI/4) * r;

      image_line(m_image, r+x1, r+y1, r+x2, r+y2, 1);
      image_line(m_image, r+x2, r+y2, r+x3, r+y3, 1);
      image_line(m_image, r+x3, r+y3, r+x4, r+y4, 1);
      image_line(m_image, r+x4, r+y4, r+x1, r+y1, 1);

      algo_floodfill(m_image, r, r, 0, m_image, algo_hline);
      break;
    }

    case PEN_TYPE_LINE: {
      double a = PI * m_angle / 180;
      int r = m_size/2;

      x =  cos(a) * r;
      y = -sin(a) * r;
      image_line(m_image, r-x, r-y, r+x, r+y, 1);
      image_line(m_image, r-x-1, r-y, r+x-1, r+y, 1);
      break;
    }
  }

  m_scanline = jnew(PenScanline, m_size);
  for (y=0; y<m_size; y++) {
    m_scanline[y].state = false;

    for (x=0; x<m_size; x++) {
      if (image_getpixel(m_image, x, y)) {
	m_scanline[y].x1 = x;

	for (; x<m_size; x++)
	  if (!image_getpixel(m_image, x, y))
	    break;

	m_scanline[y].x2 = x-1;
	m_scanline[y].state = true;
	break;
      }
    }
  }
}

