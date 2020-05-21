// Aseprite Render Library
// Copyright (c) 2019-2020 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/gradient.h"

#include "base/vector2d.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "render/dithering_matrix.h"

namespace render {

void render_rgba_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix,
  const GradientType type)
{
  switch (type) {
    case GradientType::Linear:
      render_rgba_linear_gradient(img, imgPos, p0, p1, c0, c1, matrix);
      break;
    case GradientType::Radial:
      render_rgba_radial_gradient(img, imgPos, p0, p1, c0, c1, matrix);
      break;
  }
}

void render_rgba_linear_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix)
{
  ASSERT(img->pixelFormat() == doc::IMAGE_RGB);
  if (img->pixelFormat() != doc::IMAGE_RGB) {
    return;
  }

  // If there is no vector defining the gradient (just one point),
  // the "gradient" will be just "c0"
  if (p0 == p1) {
    img->clear(c0);
    return;
  }

  base::Vector2d<double>
    u(p0.x, p0.y),
    v(p1.x, p1.y), w;
  w = v - u;
  const double wmag = w.magnitude();
  w = w.normalize();

  // As we use non-premultiplied RGB values, we need correct RGB
  // values on each stop. So in case that one color has alpha=0
  // (complete transparent), use the RGB values of the
  // non-transparent color in the other stop point.
  if (doc::rgba_geta(c0) == 0 &&
      doc::rgba_geta(c1) != 0) {
    c0 = (c1 & doc::rgba_rgb_mask);
  }
  else if (doc::rgba_geta(c0) != 0 &&
           doc::rgba_geta(c1) == 0) {
    c1 = (c0 & doc::rgba_rgb_mask);
  }

  const double r0 = double(doc::rgba_getr(c0)) / 255.0;
  const double g0 = double(doc::rgba_getg(c0)) / 255.0;
  const double b0 = double(doc::rgba_getb(c0)) / 255.0;
  const double a0 = double(doc::rgba_geta(c0)) / 255.0;

  const double r1 = double(doc::rgba_getr(c1)) / 255.0;
  const double g1 = double(doc::rgba_getg(c1)) / 255.0;
  const double b1 = double(doc::rgba_getb(c1)) / 255.0;
  const double a1 = double(doc::rgba_geta(c1)) / 255.0;

  doc::LockImageBits<doc::RgbTraits> bits(img);
  auto it = bits.begin();
  const int width = img->width();
  const int height = img->height();

  if (matrix.rows() == 1 && matrix.cols() == 1) {
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; ++x, ++it) {
        base::Vector2d<double> q(imgPos.x+x,
                                 imgPos.y+y);
        q -= u;
        double f = (q * w) / wmag;

        doc::color_t c;
        if (f < 0.0) c = c0;
        else if (f > 1.0) c = c1;
        else {
          c = doc::rgba(int(255.0 * (r0 + f*(r1-r0))),
                        int(255.0 * (g0 + f*(g1-g0))),
                        int(255.0 * (b0 + f*(b1-b0))),
                        int(255.0 * (a0 + f*(a1-a0))));
        }

        *it = c;
      }
    }
  }
  else {
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; ++x, ++it) {
        base::Vector2d<double> q(imgPos.x+x,
                                 imgPos.y+y);
        q -= u;
        double f = (q * w) / wmag;

        *it = (f*(matrix.maxValue()+2) < matrix(y, x)+1 ? c0: c1);
      }
    }
  }
}

void render_rgba_radial_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix)
{
  ASSERT(img->pixelFormat() == doc::IMAGE_RGB);
  if (img->pixelFormat() != doc::IMAGE_RGB) {
    return;
  }

  // If there is no vector defining the gradient (just one point),
  // the "gradient" will be just "c0"
  if (p0 == p1) {
    img->clear(c0);
    return;
  }

  base::Vector2d<double>
    u(p0.x, p0.y),
    v(p1.x, p1.y), w;
  w = (v - u) / 2;

  // As we use non-premultiplied RGB values, we need correct RGB
  // values on each stop. So in case that one color has alpha=0
  // (complete transparent), use the RGB values of the
  // non-transparent color in the other stop point.
  if (doc::rgba_geta(c0) == 0 &&
      doc::rgba_geta(c1) != 0) {
    c0 = (c1 & doc::rgba_rgb_mask);
  }
  else if (doc::rgba_geta(c0) != 0 &&
           doc::rgba_geta(c1) == 0) {
    c1 = (c0 & doc::rgba_rgb_mask);
  }

  const double r0 = double(doc::rgba_getr(c0)) / 255.0;
  const double g0 = double(doc::rgba_getg(c0)) / 255.0;
  const double b0 = double(doc::rgba_getb(c0)) / 255.0;
  const double a0 = double(doc::rgba_geta(c0)) / 255.0;

  const double r1 = double(doc::rgba_getr(c1)) / 255.0;
  const double g1 = double(doc::rgba_getg(c1)) / 255.0;
  const double b1 = double(doc::rgba_getb(c1)) / 255.0;
  const double a1 = double(doc::rgba_geta(c1)) / 255.0;

  doc::LockImageBits<doc::RgbTraits> bits(img);
  auto it = bits.begin();
  const int width = img->width();
  const int height = img->height();

  if (matrix.rows() == 1 && matrix.cols() == 1) {
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; ++x, ++it) {
        base::Vector2d<double> q(imgPos.x+x,
                                 imgPos.y+y);
        q -= (u+v)/2;
        q.x /= std::fabs(w.x);
        q.y /= std::fabs(w.y);
        double f = std::sqrt(q.x*q.x + q.y*q.y);

        doc::color_t c;
        if (f < 0.0) c = c0;
        else if (f > 1.0) c = c1;
        else {
          c = doc::rgba(int(255.0 * (r0 + f*(r1-r0))),
                        int(255.0 * (g0 + f*(g1-g0))),
                        int(255.0 * (b0 + f*(b1-b0))),
                        int(255.0 * (a0 + f*(a1-a0))));
        }

        *it = c;
      }
    }
  }
  else {
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; ++x, ++it) {
        base::Vector2d<double> q(imgPos.x+x,
                                 imgPos.y+y);
        q -= (u+v)/2;
        q.x /= std::fabs(w.x);
        q.y /= std::fabs(w.y);
        double f = std::sqrt(q.x*q.x + q.y*q.y);

        *it = (f*(matrix.maxValue()+2) < matrix(y, x)+1 ? c0: c1);
      }
    }
  }
}

template<typename ImageTraits>
static void create_dithering_pattern_templ(
  doc::Image* pattern,
  const render::DitheringMatrix& matrix,
  const float f,
  const doc::color_t c0,
  const doc::color_t c1)
{
  const int w = pattern->width();
  const int h = pattern->height();

  doc::LockImageBits<ImageTraits> dstBits(pattern);
  auto dst = dstBits.begin();
  for (int y=0; y<h; ++y) {
    for (int x=0; x<w; ++x, ++dst)
      *dst = (f*(matrix.maxValue()+2) < matrix(y, x)+1 ? c0: c1);
  }
}

void convert_bitmap_brush_to_dithering_brush(
  doc::Brush* brush,
  const doc::PixelFormat pixelFormat,
  const render::DitheringMatrix& matrix,
  const float f,
  const doc::color_t c0,
  const doc::color_t c1)
{
  // Create a pattern
  doc::ImageRef pattern(
    doc::Image::create(pixelFormat,
                       matrix.cols(), matrix.rows()));

  switch (pixelFormat) {
    case doc::IMAGE_RGB:
      create_dithering_pattern_templ<doc::RgbTraits>(
        pattern.get(), matrix, f, c0, c1);
      break;
    case doc::IMAGE_GRAYSCALE:
      create_dithering_pattern_templ<doc::GrayscaleTraits>(
        pattern.get(), matrix, f, c0, c1);
      break;
    case doc::IMAGE_INDEXED:
      create_dithering_pattern_templ<doc::IndexedTraits>(
        pattern.get(), matrix, f, c0, c1);
      break;
  }

  doc::ImageRef copy(doc::Image::createCopy(brush->image()));
  brush->setImage(copy.get(), copy.get());
  brush->setPatternImage(pattern);
  brush->setPattern(doc::BrushPattern::PAINT_BRUSH);
}

} // namespace render
