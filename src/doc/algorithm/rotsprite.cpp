// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/base.h"
#include "doc/algorithm/rotate.h"
#include "doc/image_impl.h"
#include "doc/primitives.h"

#include <memory>

namespace doc {
namespace algorithm {

// More information about EPX/Scale2x:
// http://en.wikipedia.org/wiki/Pixel_art_scaling_algorithms#EPX.2FScale2.C3.97.2FAdvMAME2.C3.97
// http://scale2x.sourceforge.net/algorithm.html
// http://scale2x.sourceforge.net/scale2xandepx.html
template<typename ImageTraits>
static void image_scale2x_tpl(Image* dst, const Image* src, int src_w, int src_h)
{
#if 0      // TODO complete this implementation that should be faster
           // than using a lot of get/put_pixel_fast calls.
  int dst_w = src_w*2;
  int dst_h = src_h*2;

  LockImageBits<ImageTraits> dstBits(dst, Image::WriteLock, gfx::Rect(0, 0, dst_w, dst_h));
  const LockImageBits<ImageTraits> srcBits(src);

  LockImageBits<ImageTraits>::iterator dstRow0_it = dstBits.begin();
  LockImageBits<ImageTraits>::iterator dstRow1_it = dstBits.begin();
  LockImageBits<ImageTraits>::iterator dstRow0_end = dstBits.end();
  LockImageBits<ImageTraits>::iterator dstRow1_end = dstBits.end();

  // Iterators:
  //   A
  // C P B
  //   D
  //
  // These iterators are displaced through src image and are modified in this way:
  //
  // P: is the simplest one, we just start from (0, 0) to srcEnd.
  // A: starts from row 0 (so A = P in the first row), then we start
  //    again from the row 0.
  // B: It starts from (1, row) and in the last pixel we don't moved it.
  // C: It starts from (0, 0) and then it is moved with a delay.
  // D: It starts from row 1 and continues until we reach the last
  //    row, in that case we start D iterator again.
  //
  LockImageBits<ImageTraits>::const_iterator itP, itA, itB, itC, itD, savedD;
  LockImageBits<ImageTraits>::const_iterator srcEnd = srcBits.end();
  color_t P, A, B, C, D;

  // Adjust iterators
  itP = itA = itB = itC = itD = savedD = srcBits.begin();
  dstRow1_it += dst_w;
  itD += src->width();

  for (int y=0; y<src_h; ++y) {
    if (y == 1) itA = srcBits.begin();
    if (y == src_h-2) savedD = itD;
    if (y == src_h-1) itD = savedD;
    ++itB;

    for (int x=0; x<src_w; ++x) {
      ASSERT(itP != srcEnd);
      ASSERT(itA != srcEnd);
      ASSERT(itB != srcEnd);
      ASSERT(itC != srcEnd);
      ASSERT(itD != srcEnd);
      ASSERT(dstRow0_it != dstRow0_end);
      ASSERT(dstRow1_it != dstRow1_end);

      P = *itP;
      A = *itA;                 // y-1
      B = *itB;                 // x+1
      C = *itC;                 // x-1
      D = *itD;                 // y+1

      *dstRow0_it = (C == A && C != D && A != B ? A: P);
      ++dstRow0_it;
      *dstRow0_it = (A == B && A != C && B != D ? B: P);
      ++dstRow0_it;

      *dstRow1_it = (D == C && D != B && C != A ? C: P);
      ++dstRow1_it;
      *dstRow1_it = (B == D && B != A && D != C ? D: P);
      ++dstRow1_it;

      ++itP;
      ++itA;
      if (x < src_w-2) ++itB;
      if (x > 0) ++itC;
      ++itD;
    }

    // Adjust iterators for the next two rows.
    ++itB;
    ++itC;
    dstRow0_it += dst_w;
    if (y < src_h-1)
      dstRow1_it += dst_w;
  }

  // ASSERT(itP == srcEnd);
  // ASSERT(itA == srcEnd);
  // ASSERT(itB == srcEnd);
  // ASSERT(itC == srcEnd);
  // ASSERT(itD == srcEnd);
  ASSERT(dstRow0_it == dstRow0_end);
  ASSERT(dstRow1_it == dstRow1_end);
#else

#define A c[0]
#define B c[1]
#define C c[2]
#define D c[3]
#define P c[4]

  LockImageBits<ImageTraits> dstBits(dst, gfx::Rect(0, 0, src_w*2, src_h*2));
  auto dstIt = dstBits.begin();
  auto dstIt2 = dstIt;

  color_t c[5];
  for (int y=0; y<src_h; ++y) {
    dstIt2 += src_w*2;
    for (int x=0; x<src_w; ++x) {
      P = get_pixel_fast<ImageTraits>(src, x, y);
      A = (y > 0 ? get_pixel_fast<ImageTraits>(src, x, y-1): P);
      B = (x < src_w-1 ? get_pixel_fast<ImageTraits>(src, x+1, y): P);
      C = (x > 0 ? get_pixel_fast<ImageTraits>(src, x-1, y): P);
      D = (y < src_h-1 ? get_pixel_fast<ImageTraits>(src, x, y+1): P);

      *dstIt = (C == A && C != D && A != B ? A: P);
      ++dstIt;
      *dstIt = (A == B && A != C && B != D ? B: P);
      ++dstIt;

      *dstIt2 = (D == C && D != B && C != A ? C: P);
      ++dstIt2;
      *dstIt2 = (B == D && B != A && D != C ? D: P);
      ++dstIt2;
    }
    dstIt += src_w*2;
  }

#endif
}

static void image_scale2x(Image* dst, const Image* src, int src_w, int src_h)
{
  switch (src->pixelFormat()) {
    case IMAGE_RGB:       image_scale2x_tpl<RgbTraits>(dst, src, src_w, src_h); break;
    case IMAGE_GRAYSCALE: image_scale2x_tpl<GrayscaleTraits>(dst, src, src_w, src_h); break;
    case IMAGE_INDEXED:   image_scale2x_tpl<IndexedTraits>(dst, src, src_w, src_h); break;
    case IMAGE_BITMAP:    image_scale2x_tpl<BitmapTraits>(dst, src, src_w, src_h); break;
  }
}

void rotsprite_image(Image* bmp, const Image* spr, const Image* mask,
  int x1, int y1, int x2, int y2,
  int x3, int y3, int x4, int y4)
{
  static ImageBufferPtr buf[3]; // TODO non-thread safe

  for (int i=0; i<3; ++i)
    if (!buf[i])
      buf[i].reset(new ImageBuffer(1));

  int xmin = MIN(x1, MIN(x2, MIN(x3, x4)));
  int xmax = MAX(x1, MAX(x2, MAX(x3, x4)));
  int ymin = MIN(y1, MIN(y2, MIN(y3, y4)));
  int ymax = MAX(y1, MAX(y2, MAX(y3, y4)));
  int rot_width = xmax - xmin;
  int rot_height = ymax - ymin;

  if (rot_width == 0 || rot_height == 0)
    return;

  int scale = 8;
  std::unique_ptr<Image> bmp_copy(Image::create(bmp->pixelFormat(), rot_width*scale, rot_height*scale, buf[0]));
  std::unique_ptr<Image> tmp_copy(Image::create(spr->pixelFormat(), spr->width()*scale, spr->height()*scale, buf[1]));
  std::unique_ptr<Image> spr_copy(Image::create(spr->pixelFormat(), spr->width()*scale, spr->height()*scale, buf[2]));
  std::unique_ptr<Image> msk_copy;

  color_t maskColor = spr->maskColor();

  bmp_copy->setMaskColor(maskColor);
  tmp_copy->setMaskColor(maskColor);
  spr_copy->setMaskColor(maskColor);

  spr_copy->clear(maskColor);
  spr_copy->copy(spr, gfx::Clip(spr->bounds()));

  for (int i=0; i<3; ++i) {
    // clear_image(tmp_copy, maskColor);
    image_scale2x(tmp_copy.get(), spr_copy.get(), spr->width()*(1<<i), spr->height()*(1<<i));
    spr_copy->copy(tmp_copy.get(), gfx::Clip(tmp_copy->bounds()));
  }

  if (mask) {
    // Same ImageBuffer than tmp_copy
    msk_copy.reset(Image::create(IMAGE_BITMAP, mask->width()*scale, mask->height()*scale, buf[1]));
    clear_image(msk_copy.get(), 0);
    scale_image(msk_copy.get(), mask,
                0, 0, msk_copy->width(), msk_copy->height(),
                0, 0, mask->width(), mask->height());
  }

  clear_image(bmp_copy.get(), maskColor);
  scale_image(bmp_copy.get(), bmp,
              0, 0, bmp_copy->width(), bmp_copy->height(),
              xmin, ymin, rot_width, rot_height);

  parallelogram(
    bmp_copy.get(), spr_copy.get(), msk_copy.get(),
    (x1-xmin)*scale, (y1-ymin)*scale, (x2-xmin)*scale, (y2-ymin)*scale,
    (x3-xmin)*scale, (y3-ymin)*scale, (x4-xmin)*scale, (y4-ymin)*scale);

  scale_image(bmp, bmp_copy.get(),
              xmin, ymin, rot_width, rot_height,
              0, 0, bmp_copy->width(), bmp_copy->height());
}

} // namespace algorithm
} // namespace doc
