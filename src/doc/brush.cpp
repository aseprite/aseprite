// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/brush.h"

#include "doc/algo.h"
#include "doc/algorithm/polygon.h"
#include "doc/image.h"
#include "doc/primitives.h"

#include <cmath>

namespace doc {

Brush::Brush()
{
  m_type = kCircleBrushType;
  m_size = 1;
  m_angle = 0;
  m_pattern = BrushPattern::DEFAULT;

  regenerate();
}

Brush::Brush(BrushType type, int size, int angle)
{
  m_type = type;
  m_size = size;
  m_angle = angle;
  m_pattern = BrushPattern::DEFAULT;

  regenerate();
}

Brush::Brush(const Brush& brush)
{
  m_type = brush.m_type;
  m_size = brush.m_size;
  m_angle = brush.m_angle;
  m_image = brush.m_image;
  m_pattern = brush.m_pattern;
  m_patternOrigin = brush.m_patternOrigin;

  regenerate();
}

Brush::~Brush()
{
  clean();
}

void Brush::setType(BrushType type)
{
  m_type = type;
  if (m_type != kImageBrushType)
    regenerate();
  else
    clean();
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

void Brush::setImage(const Image* image)
{
  m_type = kImageBrushType;
  m_image.reset(Image::createCopy(image));
  m_bounds = gfx::Rect(
    -m_image.get()->width()/2, -m_image.get()->height()/2,
    m_image.get()->width(), m_image.get()->height());
}

// Cleans the brush's data (image and region).
void Brush::clean()
{
  m_image.reset();
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
    size = (int)std::sqrt((double)2*m_size*m_size)+2;

  m_image.reset(Image::create(IMAGE_BITMAP, size, size));

  if (size == 1) {
    clear_image(m_image.get(), BitmapTraits::max_value);
  }
  else {
    clear_image(m_image.get(), BitmapTraits::min_value);

    switch (m_type) {

      case kCircleBrushType:
        fill_ellipse(m_image.get(), 0, 0, size-1, size-1, BitmapTraits::max_value);
        break;

      case kSquareBrushType:
        if (m_angle == 0 || size <= 2) {
          clear_image(m_image.get(), BitmapTraits::max_value);
        }
        else {
          double a = PI * m_angle / 180;
          int c = size/2;
          int r = m_size/2;
          int d = m_size;
          int x1 = int(c + r*cos(a-PI/2) + r*cos(a-PI));
          int y1 = int(c - r*sin(a-PI/2) - r*sin(a-PI));
          int x2 = int(x1 + d*cos(a));
          int y2 = int(y1 - d*sin(a));
          int x3 = int(x2 + d*cos(a+PI/2));
          int y3 = int(y2 - d*sin(a+PI/2));
          int x4 = int(x3 + d*cos(a+PI));
          int y4 = int(y3 - d*sin(a+PI));
          int points[8] = { x1, y1, x2, y2, x3, y3, x4, y4 };

          doc::algorithm::polygon(4, points, m_image.get(), algo_hline);
        }
        break;

      case kLineBrushType: {
        double a = PI * m_angle / 180;
        double r = m_size/2;
        double d = m_size;
        int x1 = int(r + r*cos(a+PI));
        int y1 = int(r - r*sin(a+PI));
        int x2 = int(x1 + d*cos(a));
        int y2 = int(y1 - d*sin(a));

        draw_line(m_image.get(), x1, y1, x2, y2, BitmapTraits::max_value);
        break;
      }
    }
  }

  m_bounds = gfx::Rect(
    -m_image->width()/2, -m_image->height()/2,
    m_image->width(), m_image->height());
}

} // namespace doc
