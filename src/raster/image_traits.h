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

#ifndef RASTER_IMAGE_TRAITS_H_INCLUDED
#define RASTER_IMAGE_TRAITS_H_INCLUDED

//////////////////////////////////////////////////////////////////////
// RGBA

#define _rgba_r_shift	0
#define _rgba_g_shift	8
#define _rgba_b_shift	16
#define _rgba_a_shift	24

inline ase_uint8 _rgba_getr(ase_uint32 c)
{
  return (c >> _rgba_r_shift) & 0xff;
}

inline ase_uint8 _rgba_getg(ase_uint32 c)
{
  return (c >> _rgba_g_shift) & 0xff;
}

inline ase_uint8 _rgba_getb(ase_uint32 c)
{
  return (c >> _rgba_b_shift) & 0xff;
}

inline ase_uint8 _rgba_geta(ase_uint32 c)
{
  return (c >> _rgba_a_shift) & 0xff;
}

inline ase_uint32 _rgba(ase_uint8 r, ase_uint8 g, ase_uint8 b, ase_uint8 a)
{
  return ((r << _rgba_r_shift) |
	  (g << _rgba_g_shift) |
	  (b << _rgba_b_shift) |
	  (a << _rgba_a_shift));
}

struct RgbTraits
{
  enum {
    imgtype = IMAGE_RGB,
    bits_per_pixel = 32,
    bytes_per_pixel = 4,
    channels = 4,
    has_alpha = true,
    is_binary = false,
  };

  typedef ase_uint32 pixel_t;
  typedef pixel_t* address_t;
  typedef const pixel_t* const_address_t;

  static inline int scanline_size(int w)
  {
    return bytes_per_pixel*w;
  }

  static inline BLEND_COLOR get_blender(int blend_mode)
  {
    ASSERT(blend_mode >= 0 && blend_mode < BLEND_MODE_MAX);
    return _rgba_blenders[blend_mode];
  }
};

//////////////////////////////////////////////////////////////////////
// Grayscale

#define _graya_v_shift	0
#define _graya_a_shift	8

inline ase_uint8 _graya_getv(ase_uint16 c)
{
  return (c >> _graya_v_shift) & 0xff;
}

inline ase_uint8 _graya_geta(ase_uint16 c)
{
  return (c >> _graya_a_shift) & 0xff;
}

inline ase_uint16 _graya(ase_uint8 v, ase_uint8 a)
{
  return ((v << _graya_v_shift) |
	  (a << _graya_a_shift));
}

struct GrayscaleTraits
{
  enum {
    imgtype = IMAGE_GRAYSCALE,
    bits_per_pixel = 16,
    bytes_per_pixel = 2,
    channels = 2,
    has_alpha = true,
    is_binary = false,
  };

  typedef ase_uint16 pixel_t;
  typedef pixel_t* address_t;
  typedef const pixel_t* const_address_t;

  static inline int scanline_size(int w)
  {
    return bytes_per_pixel*w;
  }

  static inline BLEND_COLOR get_blender(int blend_mode)
  {
    ASSERT(blend_mode >= 0 && blend_mode < BLEND_MODE_MAX);
    return _graya_blenders[blend_mode];
  }
};

//////////////////////////////////////////////////////////////////////
// Indexed

struct IndexedTraits
{
  enum {
    imgtype = IMAGE_INDEXED,
    bits_per_pixel = 8,
    bytes_per_pixel = 1,
    channels = 1,
    has_alpha = false,
    is_binary = false,
  };

  typedef ase_uint8 pixel_t;
  typedef pixel_t* address_t;
  typedef const pixel_t* const_address_t;

  static inline int scanline_size(int w)
  {
    return bytes_per_pixel*w;
  }
};

//////////////////////////////////////////////////////////////////////
// Bitmap

struct BitmapTraits
{
  enum {
    imgtype = IMAGE_BITMAP,
    bits_per_pixel = 1,
    bytes_per_pixel = 1,
    channels = 1,
    has_alpha = false,
    is_binary = true,
  };

  typedef ase_uint8 pixel_t;
  typedef pixel_t* address_t;
  typedef const pixel_t* const_address_t;

  static inline int scanline_size(int w)
  {
    return ((w+7)/8);
  }
};

#define _image_bitmap_next_bit(d, a)		\
  if (d.rem < 7)				\
    d.rem++;					\
  else {					\
    a++;					\
    d.rem = 0;					\
  }

//////////////////////////////////////////////////////////////////////

template<class Traits>
inline typename Traits::address_t image_address_fast(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->w);
  ASSERT(y >= 0 && y < image->h);

  return ((((typename Traits::pixel_t**)image->line)[y])+x);
}

template<class Traits>
inline typename Traits::pixel_t image_getpixel_fast(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->w);
  ASSERT(y >= 0 && y < image->h);

  return *((((typename Traits::pixel_t**)image->line)[y])+x);
}

template<class Traits>
inline void image_putpixel_fast(Image* image, int x, int y, typename Traits::pixel_t color)
{
  ASSERT(x >= 0 && x < image->w);
  ASSERT(y >= 0 && y < image->h);

  *((((typename Traits::pixel_t**)image->line)[y])+x) = color;
}

template<>
inline BitmapTraits::pixel_t image_getpixel_fast<BitmapTraits>(const Image* image, int x, int y)
{
  ASSERT(x >= 0 && x < image->w);
  ASSERT(y >= 0 && y < image->h);

  div_t d = div(x, 8);
  return ((*(((BitmapTraits::pixel_t**)image->line)[y]+d.quot)) & (1<<d.rem)) ? 1: 0;
}

template<>
inline void image_putpixel_fast<BitmapTraits>(Image* image, int x, int y, BitmapTraits::pixel_t color)
{
  ASSERT(x >= 0 && x < image->w);
  ASSERT(y >= 0 && y < image->h);

  div_t d = div(x, 8);
  if (color)
    *(((BitmapTraits::pixel_t**)image->line)[y]+d.quot) |= (1<<d.rem);
  else
    *(((BitmapTraits::pixel_t**)image->line)[y]+d.quot) &= ~(1<<d.rem);
}

#endif
