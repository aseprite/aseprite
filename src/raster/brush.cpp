/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/brush.h"

#include "raster/algo.h"
#include "raster/image.h"
#include "raster/primitives.h"

#include <cmath>

namespace raster {

Brush::Brush()
{
  m_type = kCircleBrushType;
  m_size = 1;
  m_angle = 0;
  m_image = NULL;

  regenerate();
}

Brush::Brush(BrushType type, int size, int angle)
{
  m_type = type;
  m_size = size;
  m_angle = angle;
  m_image = NULL;

  regenerate();
}

Brush::Brush(const Brush& brush)
{
  m_type = brush.m_type;
  m_size = brush.m_size;
  m_angle = brush.m_angle;

  regenerate();
}

Brush::~Brush()
{
  clean();
}

void Brush::setType(BrushType type)
{
  m_type = type;
  regenerate();
}

void Brush::setSize(int size)
{
  m_size = size;
  regenerate();
}

void Brush::setAngle(int angle)
{
  m_angle = angle;
  regenerate();
}

// Cleans the brush's data (image and region).
void Brush::clean()
{
  if (m_image) {
    delete m_image;
    m_image = NULL;
  }

  m_scanline.clear();
}

static void algo_hline(int x1, int y, int x2, void *data)
{
  draw_hline(reinterpret_cast<Image*>(data), x1, y, x2, BitmapTraits::max_value);
}

// Regenerates the brush bitmap and its rectangle's region.
void Brush::regenerate()
{
  clean();

  ASSERT(m_size > 0);

  int size = m_size;
  if (m_type == kSquareBrushType && m_angle != 0 && m_size > 2)
    size = std::sqrt((double)2*m_size*m_size)+2;

  m_image = Image::create(IMAGE_BITMAP, size, size);

  if (size == 1) {
    clear_image(m_image, BitmapTraits::max_value);
  }
  else {
    clear_image(m_image, BitmapTraits::min_value);

    switch (m_type) {

      case kCircleBrushType:
        fill_ellipse(m_image, 0, 0, size-1, size-1, BitmapTraits::max_value);
        break;

      case kSquareBrushType:
        if (m_angle == 0 || size <= 2) {
          clear_image(m_image, BitmapTraits::max_value);
        }
        else {
          double a = PI * m_angle / 180;
          int c = size/2;
          int r = m_size/2;
          int d = m_size;
          int x1 = c + r*cos(a-PI/2) + r*cos(a-PI);
          int y1 = c - r*sin(a-PI/2) - r*sin(a-PI);
          int x2 = x1 + d*cos(a);
          int y2 = y1 - d*sin(a);
          int x3 = x2 + d*cos(a+PI/2);
          int y3 = y2 - d*sin(a+PI/2);
          int x4 = x3 + d*cos(a+PI);
          int y4 = y3 - d*sin(a+PI);
          int points[8] = { x1, y1, x2, y2, x3, y3, x4, y4 };

          algo_polygon(4, points, m_image, algo_hline);
        }
        break;

      case kLineBrushType: {
        double a = PI * m_angle / 180;
        float r = m_size/2;
        float d = m_size;
        int x1 = r + r*cos(a+PI);
        int y1 = r - r*sin(a+PI);
        int x2 = x1 + d*cos(a);
        int y2 = y1 - d*sin(a);

        draw_line(m_image, x1, y1, x2, y2, BitmapTraits::max_value);
        break;
      }
    }
  }

  m_scanline.resize(m_image->height());
  for (int y=0; y<m_image->height(); y++) {
    m_scanline[y].state = false;

    for (int x=0; x<m_image->width(); x++) {
      if (get_pixel(m_image, x, y)) {
        m_scanline[y].x1 = x;

        for (; x<m_image->width(); x++)
          if (!get_pixel(m_image, x, y))
            break;

        m_scanline[y].x2 = x-1;
        m_scanline[y].state = true;
        break;
      }
    }
  }

  m_bounds = gfx::Rect(
    -m_image->width()/2, -m_image->height()/2,
    m_image->width(), m_image->height());
}

} // namespace raster
