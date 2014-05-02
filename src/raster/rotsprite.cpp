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

#include "base/unique_ptr.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/image_bits.h"
#include "raster/primitives.h"
#include "raster/primitives_fast.h"
#include "raster/rotate.h"

namespace raster {

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
  itD += src->getWidth();

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

  color_t c[5];
  for (int y=0; y<src_h; ++y) {
    for (int x=0; x<src_w; ++x) {
      P = get_pixel_fast<ImageTraits>(src, x, y);
      A = (y > 0 ? get_pixel_fast<ImageTraits>(src, x, y-1): P);
      B = (x < src_w-1 ? get_pixel_fast<ImageTraits>(src, x+1, y): P);
      C = (x > 0 ? get_pixel_fast<ImageTraits>(src, x-1, y): P);
      D = (y < src_h-1 ? get_pixel_fast<ImageTraits>(src, x, y+1): P);

      put_pixel_fast<ImageTraits>(dst, 2*x,   2*y,   (C == A && C != D && A != B ? A: P));
      put_pixel_fast<ImageTraits>(dst, 2*x+1, 2*y,   (A == B && A != C && B != D ? B: P));
      put_pixel_fast<ImageTraits>(dst, 2*x,   2*y+1, (D == C && D != B && C != A ? C: P));
      put_pixel_fast<ImageTraits>(dst, 2*x+1, 2*y+1, (B == D && B != A && D != C ? D: P));
    }
  }

#endif
}

static void image_scale2x(Image* dst, const Image* src, int src_w, int src_h)
{
  switch (src->getPixelFormat()) {
    case IMAGE_RGB:       image_scale2x_tpl<RgbTraits>(dst, src, src_w, src_h); break;
    case IMAGE_GRAYSCALE: image_scale2x_tpl<GrayscaleTraits>(dst, src, src_w, src_h); break;
    case IMAGE_INDEXED:   image_scale2x_tpl<IndexedTraits>(dst, src, src_w, src_h); break;
    case IMAGE_BITMAP:    image_scale2x_tpl<BitmapTraits>(dst, src, src_w, src_h); break;
  }
}

void image_rotsprite(Image* bmp, Image* spr,
                     int x1, int y1, int x2, int y2,
                     int x3, int y3, int x4, int y4)
{
  static ImageBufferPtr buf1, buf2, buf3; // TODO non-thread safe

  if (!buf1) buf1.reset(new ImageBuffer(1));
  if (!buf2) buf2.reset(new ImageBuffer(1));
  if (!buf3) buf3.reset(new ImageBuffer(1));

  int scale = 8;
  base::UniquePtr<Image> bmp_copy(Image::create(bmp->getPixelFormat(), bmp->getWidth()*scale, bmp->getHeight()*scale, buf1));
  base::UniquePtr<Image> tmp_copy(Image::create(spr->getPixelFormat(), spr->getWidth()*scale, spr->getHeight()*scale, buf2));
  base::UniquePtr<Image> spr_copy(Image::create(spr->getPixelFormat(), spr->getWidth()*scale, spr->getHeight()*scale, buf3));

  color_t maskColor = bmp->getMaskColor();

  bmp_copy->setMaskColor(maskColor);
  tmp_copy->setMaskColor(maskColor);
  spr_copy->setMaskColor(maskColor);

  bmp_copy->clear(maskColor);
  spr_copy->clear(maskColor);
  spr_copy->copy(spr, 0, 0);

  for (int i=0; i<3; ++i) {
    tmp_copy->clear(maskColor);
    image_scale2x(tmp_copy, spr_copy, spr->getWidth()*(1<<i), spr->getHeight()*(1<<i));
    spr_copy->copy(tmp_copy, 0, 0);
  }

  image_parallelogram(bmp_copy, spr_copy,
    x1*scale, y1*scale, x2*scale, y2*scale,
    x3*scale, y3*scale, x4*scale, y4*scale);

  image_scale(bmp, bmp_copy, 0, 0, bmp->getWidth(), bmp->getHeight());
}

} // namespace raster
