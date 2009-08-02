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

#include "config.h"

#include "raster/image_impl.h"

template<>
void ImageImpl<RgbTraits>::to_allegro(BITMAP *bmp, int _x, int _y) const
{
  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
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
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
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
void ImageImpl<GrayscaleTraits>::to_allegro(BITMAP *bmp, int _x, int _y) const
{
  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
      if (is_planar_bitmap(bmp)) {
        for (y=0; y<h; y++) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<w; x++) {
            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2),
		       _index_cmap[(*addr) & 0xff]);
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
            bmp_write8(bmp_address, _index_cmap[(*addr) & 0xff]);
            addr++;
            bmp_address++;
          }

          _y++;
        }
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
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
void ImageImpl<IndexedTraits>::to_allegro(BITMAP *bmp, int _x, int _y) const
{
#define RGB_TRIPLET						\
  _rgb_scale_6[_current_palette[_index_cmap[(*addr)]].r],	\
  _rgb_scale_6[_current_palette[_index_cmap[(*addr)]].g],	\
  _rgb_scale_6[_current_palette[_index_cmap[(*addr)]].b]

  const_address_t addr = raw_pixels();
  unsigned long bmp_address;
  int depth = bitmap_color_depth(bmp);
  int x, y;

  bmp_select(bmp);

  switch (depth) {

    case 8:
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
      if (is_planar_bitmap (bmp)) {
        for (y=0; y<h; y++) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<w; x++) {
            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2), _index_cmap[(*addr)]);
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
            bmp_write8(bmp_address, _index_cmap[(*addr)]);
            addr++;
            bmp_address++;
          }

          _y++;
        }
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
      }
#endif
      break;

    case 15:
      _x <<= 1;

      for (y=0; y<h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<w; x++) {
          bmp_write15(bmp_address, makecol15(RGB_TRIPLET));
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
          bmp_write16(bmp_address, makecol16(RGB_TRIPLET));
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
          bmp_write24(bmp_address, makecol24(RGB_TRIPLET));
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
          bmp_write32(bmp_address, makeacol32(RGB_TRIPLET, 255));
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
void ImageImpl<BitmapTraits>::to_allegro(BITMAP *bmp, int _x, int _y) const
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

#if defined GFX_MODEX && !defined ALLEGRO_UNIX
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
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
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
