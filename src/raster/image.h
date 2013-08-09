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

#ifndef RASTER_IMAGE_H_INCLUDED
#define RASTER_IMAGE_H_INCLUDED

#include "gfx/rect.h"
#include "raster/blend.h"
#include "raster/gfxobj.h"
#include "raster/pixel_format.h"

#include <allegro/color.h>

struct BITMAP;

namespace raster {

  class Palette;
  class Pen;
  class RgbMap;

  enum ResizeMethod {
    RESIZE_METHOD_NEAREST_NEIGHBOR,
    RESIZE_METHOD_BILINEAR,
  };

  class Image : public GfxObj {
  public:
    int w, h;
    uint8_t* dat;                 // Pixmap data.
    uint8_t** line;               // Start of each scanline.
    uint32_t mask_color;          // Skipped color in merge process.

    static Image* create(PixelFormat format, int w, int h);
    static Image* createCopy(const Image* image);

    Image(PixelFormat format, int w, int h);
    virtual ~Image();

    PixelFormat getPixelFormat() const { return m_format; }

    int getMemSize() const;

    virtual int getpixel(int x, int y) const = 0;
    virtual void putpixel(int x, int y, int color) = 0;
    virtual void clear(int color) = 0;
    virtual void copy(const Image* src, int x, int y) = 0;
    virtual void merge(const Image* src, int x, int y, int opacity, int blend_mode) = 0;
    virtual void hline(int x1, int y, int x2, int color) = 0;
    virtual void rectfill(int x1, int y1, int x2, int y2, int color) = 0;
    virtual void rectblend(int x1, int y1, int x2, int y2, int color, int opacity) = 0;
    virtual void to_allegro(BITMAP* bmp, int x, int y, const Palette* palette) const = 0;

  private:
    PixelFormat m_format;
  };

  int image_getpixel(const Image* image, int x, int y);
  void image_putpixel(Image* image, int x, int y, int color);
  void image_putpen(Image* image, Pen* pen, int x, int y, int fg_color, int bg_color);

  void image_clear(Image* image, int color);

  void image_copy(Image* dst, const Image* src, int x, int y);
  void image_merge(Image* dst, const Image* src, int x, int y, int opacity,
                   int blend_mode);

  Image* image_crop(const Image* image, int x, int y, int w, int h, int bgcolor);
  void image_rotate(const Image* src, Image* dst, int angle);

  void image_hline(Image* image, int x1, int y, int x2, int color);
  void image_vline(Image* image, int x, int y1, int y2, int color);
  void image_rect(Image* image, int x1, int y1, int x2, int y2, int color);
  void image_rectfill(Image* image, int x1, int y1, int x2, int y2, int color);
  void image_rectblend(Image* image, int x1, int y1, int x2, int y2, int color, int opacity);
  void image_line(Image* image, int x1, int y1, int x2, int y2, int color);
  void image_ellipse(Image* image, int x1, int y1, int x2, int y2, int color);
  void image_ellipsefill(Image* image, int x1, int y1, int x2, int y2, int color);

  void image_to_allegro(const Image* image, BITMAP* bmp, int x, int y, const Palette* palette);

  void image_fixup_transparent_colors(Image* image);
  void image_resize(const Image* src, Image* dst, ResizeMethod method, const Palette* palette, const RgbMap* rgbmap);
  int image_count_diff(const Image* i1, const Image* i2);
  bool image_shrink_rect(Image *image, gfx::Rect& bounds, int refpixel);

} // namespace raster

// It's here because it needs a complete definition of Image class,
// and then ImageTraits are used in the next functions below.
#include "raster/image_traits.h"

namespace raster {

  inline int pixelformat_line_size(PixelFormat pixelFormat, int width)
  {
    switch (pixelFormat) {
      case IMAGE_RGB:       return RgbTraits::scanline_size(width);
      case IMAGE_GRAYSCALE: return GrayscaleTraits::scanline_size(width);
      case IMAGE_INDEXED:   return IndexedTraits::scanline_size(width);
      case IMAGE_BITMAP:    return BitmapTraits::scanline_size(width);
    }
    return 0;
  }

  inline int image_line_size(const Image* image, int width)
  {
    return pixelformat_line_size(image->getPixelFormat(), width);
  }

  inline void* image_address(Image* image, int x, int y)
  {
    return ((void *)(image->line[y] + image_line_size(image, x)));
  }

} // namespace raster

#endif
