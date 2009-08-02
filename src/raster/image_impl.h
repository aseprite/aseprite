/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef RASTER_IMAGE_IMPL_H
#define RASTER_IMAGE_IMPL_H

#include <cassert>
#include <allegro.h>

#include "raster/image.h"

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
    assert(y >= 0 && y < h);
    return ((address_t*)line)[y];
  }

  inline const_address_t line_address(int y) const {
    assert(y >= 0 && y < h);
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

  virtual void to_allegro(BITMAP* bmp, int x, int y) const;

};

//////////////////////////////////////////////////////////////////////
// Specializations

template<>
void ImageImpl<IndexedTraits>::clear(int color)
{
  memset(raw_pixels(), color, w*h);
}

/* if "color_map" is not NULL, it's used by the routine to merge the
   source and the destionation pixels */
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
    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = ((ImageImpl<IndexedTraits>*)src)->line_address(ysrc)+xsrc;
      dst_address = ((ImageImpl<IndexedTraits>*)dst)->line_address(ydst)+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	if (*src_address) {
	  if (color_map)
	    *dst_address = color_map->data[*src_address][*dst_address];
	  else
	    *dst_address = (*src_address);
	}

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

#endif // RASTER_IMAGE_IMPL_H
