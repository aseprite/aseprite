// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/brush.h"

#include "base/pi.h"
#include "doc/algo.h"
#include "doc/algorithm/polygon.h"
#include "doc/blend_internals.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/primitives.h"

#include <algorithm>
#include <cmath>

namespace doc {

static int generation = 0;

Brush::Brush()
{
  m_type = kCircleBrushType;
  m_size = 1;
  m_angle = 0;
  m_pattern = BrushPattern::DEFAULT_FOR_UI;
  m_gen = 0;

  regenerate();
}

Brush::Brush(BrushType type, int size, int angle)
{
  m_type = type;
  m_size = size;
  m_angle = angle;
  m_pattern = BrushPattern::DEFAULT_FOR_UI;
  m_gen = 0;

  regenerate();
}

Brush::Brush(const Brush& brush)
{
  m_type = brush.m_type;
  m_size = brush.m_size;
  m_angle = brush.m_angle;
  m_image = brush.m_image;
  m_maskBitmap = brush.m_maskBitmap;
  m_pattern = brush.m_pattern;
  m_patternOrigin = brush.m_patternOrigin;
  m_gen = 0;

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

void Brush::setImage(const Image* image,
                     const Image* maskBitmap)
{
  m_type = kImageBrushType;
  m_image.reset(Image::createCopy(image));
  if (maskBitmap)
    m_maskBitmap.reset(Image::createCopy(maskBitmap));
  else {
    int w = image->width();
    int h = image->height();
    m_maskBitmap.reset(Image::create(IMAGE_BITMAP, w, h));
    LockImageBits<BitmapTraits> bits(m_maskBitmap.get());
    auto pos = bits.begin();
    for (int v=0; v<h; ++v)
      for (int u=0; u<w; ++u, ++pos)
        *pos = (get_pixel(image, u, v) != image->maskColor());
  }

  m_backupImage.reset();
  m_mainColor.reset();
  m_bgColor.reset();

  resetBounds();
}

template<class ImageTraits,
         color_t color_mask,
         color_t alpha_mask,
         color_t alpha_shift>
static void replace_image_colors(
  Image* image,
  Image* maskBitmap,
  const bool useMain, color_t mainColor,
  const bool useBg, color_t bgColor)
{
  LockImageBits<ImageTraits> bits(image, Image::ReadWriteLock);
  const LockImageBits<BitmapTraits> maskBits(maskBitmap);
  bool hasAlpha = false; // True if "image" has a pixel with alpha < 255
  color_t srcMainColor, srcBgColor;
  srcMainColor = srcBgColor = 0;

  auto mask_it = maskBits.begin();
  for (const auto& pixel : bits) {
    if (!*mask_it)
      continue;

    if ((pixel & alpha_mask) != alpha_mask) {  // If alpha != 255
      hasAlpha = true;
    }
    else if (srcBgColor == 0) {
      srcMainColor = srcBgColor = pixel;
    }
    else if (pixel != srcBgColor && srcMainColor == srcBgColor) {
      srcMainColor = pixel;
    }

    ++mask_it;
  }

  int t;
  if (hasAlpha) {
    if (useMain || useBg) {
      const color_t color = (useMain ? mainColor: useBg);
      for (auto& pixel : bits) {
        color_t a1 = (pixel & alpha_mask) >> alpha_shift;
        const color_t a2 = (color & alpha_mask) >> alpha_shift;
        a1 = MUL_UN8(a1, a2, t);
        pixel =
          (a1 << alpha_shift) |
          (color & color_mask);
      }
    }
  }
  else {
    for (auto& pixel : bits) {
      color_t color;
      if (useMain && ((pixel != srcBgColor) || (srcMainColor == srcBgColor)))
        color = mainColor;
      else if (useBg && (pixel == srcBgColor))
        color = bgColor;
      else
        continue;

      color_t a1 = (pixel & alpha_mask) >> alpha_shift;
      color_t a2 = (color & alpha_mask) >> alpha_shift;
      a1 = MUL_UN8(a1, a2, t);
      pixel =
        (a1 << alpha_shift) |
        (color & color_mask);
    }
  }
}

static void replace_image_colors_indexed(
  Image* image,
  Image* maskBitmap,
  const bool useMain, const color_t mainColor,
  const bool useBg, const color_t bgColor)
{
  LockImageBits<IndexedTraits> bits(image, Image::ReadWriteLock);
  const LockImageBits<BitmapTraits> maskBits(maskBitmap);
  bool hasAlpha = false; // True if "image" has a pixel with the mask color
  color_t maskColor = image->maskColor();
  color_t srcMainColor, srcBgColor;
  srcMainColor = srcBgColor = maskColor;

  auto mask_it = maskBits.begin();
  for (const auto& pixel : bits) {
    if (!*mask_it)
      continue;

    if (pixel == maskColor) {
      hasAlpha = true;
    }
    else if (srcBgColor == maskColor) {
      srcMainColor = srcBgColor = pixel;
    }
    else if (pixel != srcBgColor && srcMainColor == srcBgColor) {
      srcMainColor = pixel;
    }

    ++mask_it;
  }

  if (hasAlpha) {
    for (auto& pixel : bits) {
      if (pixel != maskColor) {
        if (useMain)
          pixel = mainColor;
        else if (useBg)
          pixel = bgColor;
      }
    }
  }
  else {
    for (auto& pixel : bits) {
      if (useMain && ((pixel != srcBgColor) || (srcMainColor == srcBgColor))) {
        pixel = mainColor;
      }
      else if (useBg && (pixel == srcBgColor)) {
        pixel = bgColor;
      }
    }
  }
}

void Brush::setImageColor(ImageColor imageColor, color_t color)
{
  ASSERT(m_image);
  if (!m_image)
    return;

  if (!m_backupImage)
    m_backupImage.reset(Image::createCopy(m_image.get()));
  else
    m_image.reset(Image::createCopy(m_backupImage.get()));

  ASSERT(m_maskBitmap);

  switch (imageColor) {
    case ImageColor::MainColor:
      m_mainColor.reset(new color_t(color));
      break;
    case ImageColor::BackgroundColor:
      m_bgColor.reset(new color_t(color));
      break;
  }

  switch (m_image->pixelFormat()) {

    case IMAGE_RGB:
      replace_image_colors<RgbTraits, rgba_rgb_mask, rgba_a_mask, rgba_a_shift>(
        m_image.get(), m_maskBitmap.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;

    case IMAGE_GRAYSCALE:
      replace_image_colors<GrayscaleTraits, graya_v_mask, graya_a_mask, graya_a_shift>(
        m_image.get(), m_maskBitmap.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;

    case IMAGE_INDEXED:
      replace_image_colors_indexed(
        m_image.get(), m_maskBitmap.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;
  }
}

void Brush::setCenter(const gfx::Point& center)
{
  m_center = center;
  m_bounds = gfx::Rect(-m_center,
                       gfx::Size(m_image->width(),
                                 m_image->height()));
}

// Cleans the brush's data (image and region).
void Brush::clean()
{
  m_gen = ++generation;
  m_image.reset();
  m_maskBitmap.reset();
  m_backupImage.reset();
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
  m_maskBitmap.reset();

  resetBounds();

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
        const double a = PI * m_angle / 180;
        const double r = m_size / 2.0;
        const int cx = m_center.x;
        const int cy = m_center.y;
        const int dx = int(r*cos(-a));
        const int dy = int(r*sin(-a));

        draw_line(m_image.get(), cx, cy, cx+dx, cy+dy, BitmapTraits::max_value);
        draw_line(m_image.get(), cx, cy, cx-dx, cy-dy, BitmapTraits::max_value);
        break;
      }
    }
  }
}

void Brush::resetBounds()
{
  m_center = gfx::Point(std::max(0, m_image->width()/2),
                        std::max(0, m_image->height()/2));
  m_bounds = gfx::Rect(-m_center,
                       gfx::Size(m_image->width(),
                                 m_image->height()));
}

} // namespace doc
