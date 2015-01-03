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

#include "raster/conversion_alleg.h"

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/color_scales.h"
#include "raster/image.h"
#include "raster/image_impl.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

#include <allegro.h>

namespace raster {

namespace {

template<typename ImageTraits, int color_depth>
int convert_color_to_allegro(color_t color, const Palette* palette) {
  static_assert(false && sizeof(ImageTraits), "Invalid color conversion");
  return 0;
}

template<>
int convert_color_to_allegro<RgbTraits, 8>(color_t c, const Palette* palette) {
  return makecol8(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<RgbTraits, 15>(color_t c, const Palette* palette) {
  return makecol15(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<RgbTraits, 16>(color_t c, const Palette* palette) {
  return makecol16(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<RgbTraits, 24>(color_t c, const Palette* palette) {
  return makecol24(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<RgbTraits, 32>(color_t c, const Palette* palette) {
  return makeacol32(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c));
}

template<>
int convert_color_to_allegro<GrayscaleTraits, 8>(color_t c, const Palette* palette) {
  return makecol8(graya_getv(c), graya_getv(c), graya_getv(c));
}
template<>
int convert_color_to_allegro<GrayscaleTraits, 15>(color_t c, const Palette* palette) {
  return makecol15(graya_getv(c), graya_getv(c), graya_getv(c));
}
template<>
int convert_color_to_allegro<GrayscaleTraits, 16>(color_t c, const Palette* palette) {
  return makecol16(graya_getv(c), graya_getv(c), graya_getv(c));
}
template<>
int convert_color_to_allegro<GrayscaleTraits, 24>(color_t c, const Palette* palette) {
  return makecol24(graya_getv(c), graya_getv(c), graya_getv(c));
}
template<>
int convert_color_to_allegro<GrayscaleTraits, 32>(color_t c, const Palette* palette) {
  return makeacol32(graya_getv(c), graya_getv(c), graya_getv(c), graya_geta(c));
}

template<>
int convert_color_to_allegro<IndexedTraits, 8>(color_t color, const Palette* palette) {
  return color;
}
template<>
int convert_color_to_allegro<IndexedTraits, 15>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol15(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<IndexedTraits, 16>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol16(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<IndexedTraits, 24>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol24(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<IndexedTraits, 32>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makeacol32(rgba_getr(c), rgba_getg(c), rgba_getb(c), 255);
}

template<>
int convert_color_to_allegro<BitmapTraits, 8>(color_t color, const Palette* palette) {
  return color;
}
template<>
int convert_color_to_allegro<BitmapTraits, 15>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol15(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<BitmapTraits, 16>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol16(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<BitmapTraits, 24>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makecol24(rgba_getr(c), rgba_getg(c), rgba_getb(c));
}
template<>
int convert_color_to_allegro<BitmapTraits, 32>(color_t color, const Palette* palette) {
  color_t c = palette->getEntry(color);
  return makeacol32(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c));
}

template<typename ImageTraits>
void convert_image_to_allegro_templ(const Image* image, BITMAP* bmp, int _x, int _y, const Palette* palette)
{
  const LockImageBits<ImageTraits> bits(image);
  typename LockImageBits<ImageTraits>::const_iterator src_it = bits.begin();
#ifdef _DEBUG
  typename LockImageBits<ImageTraits>::const_iterator src_end = bits.end();
#endif
  int depth = bitmap_color_depth(bmp);
  int x, y, w = image->width(), h = image->height();
  unsigned long bmp_address;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      if (is_planar_bitmap (bmp)) {
        for (y=0; y<h; ++y) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<w; ++x) {
            ASSERT(src_it != src_end);

            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2),
                       (convert_color_to_allegro<ImageTraits, 8>(*src_it, palette)));

            ++src_it;
            address++;
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<h; ++y) {
          bmp_address = bmp_write_line(bmp, _y)+_x;

          for (x=0; x<w; ++x) {
            ASSERT(src_it != src_end);

            bmp_write8(bmp_address,
                       (convert_color_to_allegro<ImageTraits, 8>(*src_it, palette)));

            ++bmp_address;
            ++src_it;
          }

          _y++;
        }
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      }
#endif
      break;

    case 15:
      _x <<= 1;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; ++x) {
          ASSERT(src_it != src_end);

          bmp_write15(bmp_address,
                      (convert_color_to_allegro<ImageTraits, 15>(*src_it, palette)));

          bmp_address += 2;
          ++src_it;
        }

        _y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<h; ++y) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; ++x) {
          ASSERT(src_it != src_end);

          bmp_write16(bmp_address,
                      (convert_color_to_allegro<ImageTraits, 16>(*src_it, palette)));

          bmp_address += 2;
          ++src_it;
        }

        _y++;
      }
      break;

    case 24:
      _x *= 3;

      for (y=0; y<h; ++y) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; ++x) {
          ASSERT(src_it != src_end);

          bmp_write24(bmp_address,
                      (convert_color_to_allegro<ImageTraits, 24>(*src_it, palette)));

          bmp_address += 3;
          ++src_it;
        }

        _y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<h; ++y) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; ++x) {
          ASSERT(src_it != src_end);

          bmp_write32(bmp_address,
                      (convert_color_to_allegro<ImageTraits, 32>(*src_it, palette)));

          bmp_address += 4;
          ++src_it;
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

} // anonymous namespace

void convert_image_to_allegro(const Image* image, BITMAP* bmp, int x, int y, const Palette* palette)
{
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       convert_image_to_allegro_templ<RgbTraits>(image, bmp, x, y, palette); break;
    case IMAGE_GRAYSCALE: convert_image_to_allegro_templ<GrayscaleTraits>(image, bmp, x, y, palette); break;
    case IMAGE_INDEXED:   convert_image_to_allegro_templ<IndexedTraits>(image, bmp, x, y, palette); break;
    case IMAGE_BITMAP:    convert_image_to_allegro_templ<BitmapTraits>(image, bmp, x, y, palette); break;
  }
}

void convert_palette_to_allegro(const Palette* palette, RGB* rgb)
{
  int i;
  for (i=0; i<palette->size(); ++i) {
    color_t c = palette->getEntry(i);
    rgb[i].r = rgba_getr(c) / 4;
    rgb[i].g = rgba_getg(c) / 4;
    rgb[i].b = rgba_getb(c) / 4;
  }
  for (; i<Palette::MaxColors; ++i) {
    rgb[i].r = 0;
    rgb[i].g = 0;
    rgb[i].b = 0;
  }
}

void convert_palette_from_allegro(const RGB* rgb, Palette* palette)
{
  palette->resize(Palette::MaxColors);
  for (int i=0; i<Palette::MaxColors; ++i) {
    palette->setEntry(i, rgba(scale_6bits_to_8bits(rgb[i].r),
                              scale_6bits_to_8bits(rgb[i].g),
                              scale_6bits_to_8bits(rgb[i].b), 255));
  }
}

} // namespace raster
