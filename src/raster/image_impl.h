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

#ifndef RASTER_IMAGE_IMPL_H_INCLUDED
#define RASTER_IMAGE_IMPL_H_INCLUDED

#include "raster/image.h"
#include "raster/palette.h"

template<class Traits>
class ImageImpl : public Image
{
  typedef typename Traits::address_t address_t;
  typedef typename Traits::const_address_t const_address_t;

public:				// raw access to pixel-data

  inline address_t raw_pixels() {
    return (address_t)dat;
  }

  inline const_address_t raw_pixels() const {
    return (address_t)dat;
  }

  inline address_t line_address(int y) {
    ASSERT(y >= 0 && y < h);
    return ((address_t*)line)[y];
  }

  inline const_address_t line_address(int y) const {
    ASSERT(y >= 0 && y < h);
    return ((const_address_t*)line)[y];
  }
  
public:

  ImageImpl(int w, int h)
    : Image(Traits::imgtype, w, h)
  {
    int bytes_per_line = Traits::scanline_size(w);

    dat = new ase_uint8[bytes_per_line*h];
    try {
      line = (ase_uint8**)new address_t*[h];
    }
    catch (...) {
      delete[] dat;
      throw;
    }

    address_t addr = raw_pixels();
    for (int y=0; y<h; ++y) {
      ((address_t*)line)[y] = addr;
      addr = (address_t)(((ase_uint8*)addr) + bytes_per_line);
    }
  }

  virtual int getpixel(int x, int y) const
  {
    return image_getpixel_fast<Traits>(this, x, y);
  }

  virtual void putpixel(int x, int y, int color)
  {
    image_putpixel_fast<Traits>(this, x, y, color);
  }

  virtual void clear(int color)
  {
    address_t addr = raw_pixels();
    unsigned int c, size = w*h;

    for (c=0; c<size; c++)
      *(addr++) = color;
  }

  virtual void copy(const Image* src, int x, int y)
  {
    Image* dst = this;
    address_t src_address;
    address_t dst_address;
    int xbeg, xend, xsrc;
    int ybeg, yend, ysrc, ydst;
    int bytes;

    // clipping

    xsrc = 0;
    ysrc = 0;

    xbeg = x;
    ybeg = y;
    xend = x+src->w-1;
    yend = y+src->h-1;

    if ((xend < 0) || (xbeg >= dst->w) ||
	(yend < 0) || (ybeg >= dst->h))
      return;

    if (xbeg < 0) {
      xsrc -= xbeg;
      xbeg = 0;
    }

    if (ybeg < 0) {
      ysrc -= ybeg;
      ybeg = 0;
    }

    if (xend >= dst->w)
      xend = dst->w-1;

    if (yend >= dst->h)
      yend = dst->h-1;

    // copy process

    bytes = Traits::scanline_size(xend - xbeg + 1);

    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = ((ImageImpl<Traits>*)src)->line_address(ysrc)+xsrc;
      dst_address = ((ImageImpl<Traits>*)dst)->line_address(ydst)+xbeg;

      memcpy(dst_address, src_address, bytes);
    }
  }

  virtual void merge(const Image* src, int x, int y, int opacity, int blend_mode)
  {
    BLEND_COLOR blender = Traits::get_blender(blend_mode);
    register ase_uint32 mask_color = src->mask_color;
    Image* dst = this;
    address_t src_address;
    address_t dst_address;
    int xbeg, xend, xsrc, xdst;
    int ybeg, yend, ysrc, ydst;

    // nothing to do
    if (!opacity)
      return;

    // clipping

    xsrc = 0;
    ysrc = 0;

    xbeg = x;
    ybeg = y;
    xend = x+src->w-1;
    yend = y+src->h-1;

    if ((xend < 0) || (xbeg >= dst->w) ||
	(yend < 0) || (ybeg >= dst->h))
      return;

    if (xbeg < 0) {
      xsrc -= xbeg;
      xbeg = 0;
    }

    if (ybeg < 0) {
      ysrc -= ybeg;
      ybeg = 0;
    }

    if (xend >= dst->w)
      xend = dst->w-1;

    if (yend >= dst->h)
      yend = dst->h-1;

    // merge process

    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = ((ImageImpl<Traits>*)src)->line_address(ysrc)+xsrc;
      dst_address = ((ImageImpl<Traits>*)dst)->line_address(ydst)+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	if (*src_address != mask_color)
	  *dst_address = (*blender)(*dst_address, *src_address, opacity);

	dst_address++;
	src_address++;
      }
    }
  }

  virtual void hline(int x1, int y, int x2, int color)
  {
    address_t addr = line_address(y)+x1;

    for (int x=x1; x<=x2; ++x)
      *(addr++) = color;
  }

  virtual void rectfill(int x1, int y1, int x2, int y2, int color)
  {
    address_t addr;
    int x, y;

    for (y=y1; y<=y2; ++y) {
      addr = line_address(y)+x1;
      for (x=x1; x<=x2; ++x)
	*(addr++) = color;
    }
  }

  virtual void to_allegro(BITMAP* bmp, int x, int y, const Palette* palette) const;

};

//////////////////////////////////////////////////////////////////////
// Specializations

template<>
void ImageImpl<IndexedTraits>::clear(int color)
{
  memset(raw_pixels(), color, w*h);
}

template<>
void ImageImpl<IndexedTraits>::merge(const Image* src, int x, int y, int opacity, int blend_mode)
{
  Image* dst = this;
  address_t src_address;
  address_t dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;

  // clipping

  xsrc = 0;
  ysrc = 0;

  xbeg = x;
  ybeg = y;
  xend = x+src->w-1;
  yend = y+src->h-1;

  if ((xend < 0) || (xbeg >= dst->w) ||
      (yend < 0) || (ybeg >= dst->h))
    return;

  if (xbeg < 0) {
    xsrc -= xbeg;
    xbeg = 0;
  }

  if (ybeg < 0) {
    ysrc -= ybeg;
    ybeg = 0;
  }

  if (xend >= dst->w)
    xend = dst->w-1;

  if (yend >= dst->h)
    yend = dst->h-1;

  // merge process

  // direct copy
  if (blend_mode == BLEND_MODE_COPY) {
    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = ((ImageImpl<IndexedTraits>*)src)->line_address(ysrc)+xsrc;
      dst_address = ((ImageImpl<IndexedTraits>*)dst)->line_address(ydst)+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	*dst_address = (*src_address);

	dst_address++;
	src_address++;
      }
    }
  }
  // with mask
  else {
    register int mask_color = src->mask_color;

    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = ((ImageImpl<IndexedTraits>*)src)->line_address(ysrc)+xsrc;
      dst_address = ((ImageImpl<IndexedTraits>*)dst)->line_address(ydst)+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	if (*src_address != mask_color)
	  *dst_address = (*src_address);

	dst_address++;
	src_address++;
      }
    }
  }
}

template<>
void ImageImpl<BitmapTraits>::clear(int color)
{
  memset(raw_pixels(), color ? 0xff: 0x00, ((w+7)/8) * h);
}

#define BITMAP_HLINE(op)			\
  for (x=x1; x<=x2; x++) {			\
    *addr op (1<<d.rem);			\
    _image_bitmap_next_bit(d, addr);		\
  }

template<>
void ImageImpl<BitmapTraits>::hline(int x1, int y, int x2, int color)
{
  div_t d = div(x1, 8);
  address_t addr = line_address(y)+d.quot;
  int x;

  if (color) {
    BITMAP_HLINE( |= );
  }
  else {
    BITMAP_HLINE( &= ~ );
  }
}

template<>
void ImageImpl<BitmapTraits>::rectfill(int x1, int y1, int x2, int y2, int color)
{
  div_t d, beg_d = div(x1, 8);
  address_t addr;
  int x, y;

  if (color) {
    for (y=y1; y<=y2; y++) {
      d = beg_d;
      addr = line_address(y)+d.quot;
      BITMAP_HLINE( |= );
    }
  }
  else {
    for (y=y1; y<=y2; y++) {
      d = beg_d;
      addr = line_address(y)+d.quot;
      BITMAP_HLINE( &= ~ );
    }
  }
}

template<>
void ImageImpl<BitmapTraits>::copy(const Image* src, int x, int y)
{
  Image* dst = this;
  address_t src_address;
  address_t dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;
  div_t src_d, src_beg_d;
  div_t dst_d, dst_beg_d;

  // clipping

  xsrc = 0;
  ysrc = 0;

  xbeg = x;
  ybeg = y;
  xend = x+src->w-1;
  yend = y+src->h-1;

  if ((xend < 0) || (xbeg >= dst->w) ||
      (yend < 0) || (ybeg >= dst->h))
    return;

  if (xbeg < 0) {
    xsrc -= xbeg;
    xbeg = 0;
  }

  if (ybeg < 0) {
    ysrc -= ybeg;
    ybeg = 0;
  }

  if (xend >= dst->w)
    xend = dst->w-1;

  if (yend >= dst->h)
    yend = dst->h-1;

  // copy process

  src_beg_d = div(xsrc, 8);
  dst_beg_d = div(xbeg, 8);

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_d = src_beg_d;
    dst_d = dst_beg_d;

    src_address = ((ImageImpl<BitmapTraits>*)src)->line_address(ysrc)+src_d.quot;
    dst_address = ((ImageImpl<BitmapTraits>*)dst)->line_address(ydst)+dst_d.quot;

    for (xdst=xbeg; xdst<=xend; xdst++) {
      if ((*src_address & (1<<src_d.rem)))
        *dst_address |= (1<<dst_d.rem);
      else
        *dst_address &= ~(1<<dst_d.rem);

      _image_bitmap_next_bit(src_d, src_address);
      _image_bitmap_next_bit(dst_d, dst_address);
    }
  }
}

template<>
void ImageImpl<BitmapTraits>::merge(const Image* src, int x, int y, int opacity, int blend_mode)
{
  Image* dst = this;
  address_t src_address;
  address_t dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;
  div_t src_d, src_beg_d;
  div_t dst_d, dst_beg_d;

  // clipping

  xsrc = 0;
  ysrc = 0;

  xbeg = x;
  ybeg = y;
  xend = x+src->w-1;
  yend = y+src->h-1;

  if ((xend < 0) || (xbeg >= dst->w) ||
      (yend < 0) || (ybeg >= dst->h))
    return;

  if (xbeg < 0) {
    xsrc -= xbeg;
    xbeg = 0;
  }

  if (ybeg < 0) {
    ysrc -= ybeg;
    ybeg = 0;
  }

  if (xend >= dst->w)
    xend = dst->w-1;

  if (yend >= dst->h)
    yend = dst->h-1;

  // copy process

  src_beg_d = div(xsrc, 8);
  dst_beg_d = div(xbeg, 8);

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_d = src_beg_d;
    dst_d = dst_beg_d;

    src_address = ((ImageImpl<BitmapTraits>*)src)->line_address(ysrc)+src_d.quot;
    dst_address = ((ImageImpl<BitmapTraits>*)dst)->line_address(ydst)+dst_d.quot;

    for (xdst=xbeg; xdst<=xend; xdst++) {
      if ((*src_address & (1<<src_d.rem)))
        *dst_address |= (1<<dst_d.rem);

      _image_bitmap_next_bit(src_d, src_address);
      _image_bitmap_next_bit(dst_d, dst_address);
    }
  }
}

template<>
void ImageImpl<RgbTraits>::to_allegro(BITMAP *bmp, int _x, int _y, const Palette* palette) const
{
  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      if (is_planar_bitmap(bmp)) {
	for (y=0; y<h; y++) {
	  bmp_address = (unsigned long)bmp->line[_y];

	  for (x=0; x<image->w; x++) {
	    outportw(0x3C4, (0x100<<((_x+x)&3))|2);
	    bmp_write8(bmp_address+((_x+x)>>2),
		       makecol8((*addr) & 0xff,
				((*addr)>>8) & 0xff,
				((*addr)>>16) & 0xff));
	    addr++;
	  }
  
	  _y++;
	}
      }
      else {
#endif
	for (y=0; y<h; y++) {
	  bmp_address = bmp_write_line(bmp, _y)+_x;
  
	  for (x=0; x<w; x++) {
	    bmp_write8(bmp_address,
		       makecol8((*addr) & 0xff,
				((*addr)>>8) & 0xff,
				((*addr)>>16) & 0xff));
	    addr++;
	    bmp_address++;
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

	for (x=0; x<w; x++) {
	  bmp_write15(bmp_address,
		      makecol15((*addr) & 0xff,
				((*addr)>>8) & 0xff,
				((*addr)>>16) & 0xff));
	  addr++;
	  bmp_address += 2;
	}

	_y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<h; y++) {
	bmp_address = bmp_write_line (bmp, _y)+_x;

	for (x=0; x<w; x++) {
	  bmp_write16(bmp_address,
		      makecol16((*addr) & 0xff,
				((*addr)>>8) & 0xff,
				((*addr)>>16) & 0xff));
	  addr++;
	  bmp_address += 2;
	}

	_y++;
      }
      break;

    case 24:
      _x *= 3;

      for (y=0; y<h; y++) {
	bmp_address = bmp_write_line(bmp, _y)+_x;

	for (x=0; x<w; x++) {
	  bmp_write24(bmp_address,
		      makecol24((*addr) & 0xff,
				((*addr)>>8) & 0xff,
				((*addr)>>16) & 0xff));
	  addr++;
	  bmp_address += 3;
	}

	_y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<h; y++) {
	bmp_address = bmp_write_line(bmp, _y)+_x;

	for (x=0; x<w; x++) {
	  bmp_write32(bmp_address,
		      makeacol32((*addr) & 0xff,
				 ((*addr)>>8) & 0xff,
				 ((*addr)>>16) & 0xff,
				 ((*addr)>>24) & 0xff));
	  addr++;
	  bmp_address += 4;
	}

	_y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

template<>
void ImageImpl<GrayscaleTraits>::to_allegro(BITMAP *bmp, int _x, int _y, const Palette* palette) const
{
  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      if (is_planar_bitmap(bmp)) {
        for (y=0; y<h; y++) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<w; x++) {
            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2), (*addr) & 0xff);
            addr++;
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<h; y++) {
          bmp_address = bmp_write_line(bmp, _y)+_x;

          for (x=0; x<w; x++) {
            bmp_write8(bmp_address, (*addr) & 0xff);
            addr++;
            bmp_address++;
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

        for (x=0; x<w; x++) {
          bmp_write15(bmp_address,
		      makecol15((*addr) & 0xff,
				(*addr) & 0xff,
				(*addr) & 0xff));
          addr++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
          bmp_write16(bmp_address,
		      makecol16((*addr) & 0xff,
				(*addr) & 0xff,
				(*addr) & 0xff));
          addr++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 24:
      _x *= 3;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
          bmp_write24(bmp_address,
		      makecol24((*addr) & 0xff,
				(*addr) & 0xff,
				(*addr) & 0xff));
          addr++;
          bmp_address += 3;
        }

        _y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
          bmp_write32(bmp_address,
		      makeacol32((*addr) & 0xff,
				 (*addr) & 0xff,
				 (*addr) & 0xff, 255));
          addr++;
          bmp_address += 4;
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

template<>
void ImageImpl<IndexedTraits>::to_allegro(BITMAP *bmp, int _x, int _y, const Palette* palette) const
{
  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;
  ase_uint32 c;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      if (is_planar_bitmap (bmp)) {
        for (y=0; y<h; y++) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<w; x++) {
            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2), (*addr));
            address++;
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<h; y++) {
          bmp_address = bmp_write_line(bmp, _y)+_x;

          for (x=0; x<w; x++) {
            bmp_write8(bmp_address, (*addr));
            addr++;
            bmp_address++;
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

        for (x=0; x<w; x++) {
	  c = palette->getEntry(*addr);
          bmp_write15(bmp_address, makecol15(_rgba_getr(c), _rgba_getg(c), _rgba_getb(c)));
          addr++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
	  c = palette->getEntry(*addr);
          bmp_write16(bmp_address, makecol16(_rgba_getr(c), _rgba_getg(c), _rgba_getb(c)));
          addr++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 24:
      _x *= 3;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
	  c = palette->getEntry(*addr);
          bmp_write24(bmp_address, makecol24(_rgba_getr(c), _rgba_getg(c), _rgba_getb(c)));
          addr++;
          bmp_address += 3;
        }

        _y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
	  c = palette->getEntry(*addr);
          bmp_write32(bmp_address, makeacol32(_rgba_getr(c), _rgba_getg(c), _rgba_getb(c), 255));
          addr++;
          bmp_address += 4;
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

template<>
void ImageImpl<BitmapTraits>::to_allegro(BITMAP *bmp, int _x, int _y, const Palette* palette) const
{
  const_address_t addr;
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  div_t d, beg_d = div(0, 8);
  int color[2];
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
      color[0] = makecol8(0, 0, 0);
      color[1] = makecol8(255, 255, 255);

#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      if (is_planar_bitmap(bmp)) {
        for (y=0; y<h; y++) {
	  addr = line_address(y);
          bmp_address = (unsigned long)bmp->line[_y];

	  d = beg_d;
          for (x=0; x<w; x++) {
            outportw (0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_addr+((_x+x)>>2),
		       color[((*addr) & (1<<d.rem))? 1: 0]);
	    _image_bitmap_next_bit(d, addr);
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<h; y++) {
	  addr = line_address(y);
          bmp_address = bmp_write_line(bmp, _y)+_x;

	  d = beg_d;
          for (x=0; x<w; x++) {
            bmp_write8 (bmp_address++, color[((*addr) & (1<<d.rem))? 1: 0]);
	    _image_bitmap_next_bit(d, addr);
          }

          _y++;
        }
#if defined GFX_MODEX && !defined ALLEGRO_UNIX && !defined ALLEGRO_MACOSX
      }
#endif
      break;

    case 15:
      color[0] = makecol15(0, 0, 0);
      color[1] = makecol15(255, 255, 255);

      _x <<= 1;

      for (y=0; y<h; y++) {
	addr = line_address(y);
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<w; x++) {
          bmp_write15(bmp_address, color[((*addr) & (1<<d.rem))? 1: 0]);
          bmp_address += 2;
	  _image_bitmap_next_bit(d, addr);
        }

        _y++;
      }
      break;

    case 16:
      color[0] = makecol16(0, 0, 0);
      color[1] = makecol16(255, 255, 255);

      _x <<= 1;

      for (y=0; y<h; y++) {
	addr = line_address(y);
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<w; x++) {
          bmp_write16(bmp_address, color[((*addr) & (1<<d.rem))? 1: 0]);
          bmp_address += 2;
	  _image_bitmap_next_bit(d, addr);
        }

        _y++;
      }
      break;

    case 24:
      color[0] = makecol24 (0, 0, 0);
      color[1] = makecol24 (255, 255, 255);

      _x *= 3;

      for (y=0; y<h; y++) {
	addr = line_address(y);
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<w; x++) {
          bmp_write24(bmp_address, color[((*addr) & (1<<d.rem))? 1: 0]);
          bmp_address += 3;
	  _image_bitmap_next_bit (d, addr);
        }

        _y++;
      }
      break;

    case 32:
      color[0] = makeacol32 (0, 0, 0, 255);
      color[1] = makeacol32 (255, 255, 255, 255);

      _x <<= 2;

      for (y=0; y<h; y++) {
	addr = line_address(y);
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<w; x++) {
          bmp_write32(bmp_address, color[((*addr) & (1<<d.rem))? 1: 0]);
          bmp_address += 4;
	  _image_bitmap_next_bit(d, addr);
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

#endif
