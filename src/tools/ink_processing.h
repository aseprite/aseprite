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

#include "effect/effect.h"
#include "modules/palettes.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"
#include "raster/sprite.h"

//////////////////////////////////////////////////////////////////////
// Ink Processing
//////////////////////////////////////////////////////////////////////

#define DEFINE_INK_PROCESSING(addresses_define,				\
			      addresses_initialize,			\
			      addresses_increment,			\
			      processing)				\
  addresses_define							\
  register int x;							\
									\
  /* with mask */							\
  if (!loop->getMask()->is_empty()) {					\
    Point maskOrigin(loop->getMaskOrigin());				\
									\
    if ((y < maskOrigin.y) || (y >= maskOrigin.y+loop->getMask()->h))	\
      return;								\
									\
    if (x1 < maskOrigin.x)						\
      x1 = maskOrigin.x;						\
									\
    if (x2 > maskOrigin.x+loop->getMask()->w-1)				\
      x2 = maskOrigin.x+loop->getMask()->w-1;				\
									\
    if (Image* bitmap = loop->getMask()->bitmap) {			\
      addresses_initialize;						\
      for (x=x1; x<=x2; ++x) {						\
	if (bitmap->getpixel(x-maskOrigin.x, y-maskOrigin.y))		\
	  processing;							\
									\
	addresses_increment;						\
      }									\
      return;								\
    }									\
  }									\
									\
  addresses_initialize;							\
  for (x=x1; x<=x2; ++x) {						\
    processing;								\
    addresses_increment;						\
  }

#define DEFINE_INK_PROCESSING_DST(Traits, processing)			\
  DEFINE_INK_PROCESSING(register Traits::address_t dst_address;				 , \
			dst_address = ((Traits::address_t*)loop->getDstImage()->line)[y]+x1; , \
			++dst_address							 , \
			processing)

#define DEFINE_INK_PROCESSING_SRCDST(Traits, processing)			  \
  DEFINE_INK_PROCESSING(register Traits::address_t src_address;			  \
			register Traits::address_t dst_address;			, \
			src_address = ((Traits::address_t*)loop->getSrcImage()->line)[y]+x1;   \
			dst_address = ((Traits::address_t*)loop->getDstImage()->line)[y]+x1; , \
			++src_address;						  \
			++dst_address;						, \
			processing)

//////////////////////////////////////////////////////////////////////
// Opaque Ink
//////////////////////////////////////////////////////////////////////

static void ink_hline32_opaque(int x1, int y, int x2, IToolLoop* loop)
{
  int c = loop->getPrimaryColor();

  DEFINE_INK_PROCESSING_DST
    (RgbTraits,
     *dst_address = c			);
}

static void ink_hline16_opaque(int x1, int y, int x2, IToolLoop* loop)
{
  int c = loop->getPrimaryColor();

  DEFINE_INK_PROCESSING_DST
    (GrayscaleTraits,
     *dst_address = c			);
}

static void ink_hline8_opaque(int x1, int y, int x2, IToolLoop* loop)
{
  int c = loop->getPrimaryColor();

  DEFINE_INK_PROCESSING_DST
    (IndexedTraits,
     *dst_address = c			);

  /* memset(((ase_uint8 **)data->dst_image->line)[y]+x1, data->color, x2-x1+1); */
}

//////////////////////////////////////////////////////////////////////
// Transparent Ink
//////////////////////////////////////////////////////////////////////

static void ink_hline32_transparent(int x1, int y, int x2, IToolLoop* loop)
{
  int color = loop->getPrimaryColor();
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (RgbTraits,
     *dst_address = _rgba_blend_normal(*src_address, color, opacity));
}  

static void ink_hline16_transparent(int x1, int y, int x2, IToolLoop* loop)
{
  int color = loop->getPrimaryColor();
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (GrayscaleTraits,
     *dst_address = _graya_blend_normal(*src_address, color, opacity));
}  

static void ink_hline8_transparent(int x1, int y, int x2, IToolLoop* loop)
{
  Palette* pal = get_current_palette();
  RgbMap* rgbmap = loop->getSprite()->getRgbMap();
  ase_uint32 c;
  ase_uint32 tc = pal->getEntry(loop->getPrimaryColor());
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (IndexedTraits,
     {
       c = _rgba_blend_normal(pal->getEntry(*src_address), tc, opacity);
       *dst_address = rgbmap->mapColor(_rgba_getr(c),
				       _rgba_getg(c),
				       _rgba_getb(c));
     });
}  

//////////////////////////////////////////////////////////////////////
// Blur Ink
//////////////////////////////////////////////////////////////////////

static void ink_hline32_blur(int x1, int y, int x2, IToolLoop* loop)
{
  int c, r, g, b, a;
  int opacity = loop->getOpacity();
  TiledMode tiled = loop->getTiledMode();
  Image* src = loop->getSrcImage();
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint32 *src_address2;

  DEFINE_INK_PROCESSING_SRCDST
    (RgbTraits,
     {
       c = 0;
       r = g = b = a = 0;

       GET_MATRIX_DATA
	 (ase_uint32, src, src_address2,
	  3, 3, 1, 1, tiled,
	  color = *src_address2;
	  if (_rgba_geta(color) != 0) {
	    r += _rgba_getr(color);
	    g += _rgba_getg(color);
	    b += _rgba_getb(color);
	    a += _rgba_geta(color);
	    ++c;
	  }
	  );

       if (c > 0) {
	 r /= c;
	 g /= c;
	 b /= c;
	 a /= 9;

	 c = *src_address;
	 r = _rgba_getr(c) + (r-_rgba_getr(c)) * opacity / 255;
	 g = _rgba_getg(c) + (g-_rgba_getg(c)) * opacity / 255;
	 b = _rgba_getb(c) + (b-_rgba_getb(c)) * opacity / 255;
	 a = _rgba_geta(c) + (a-_rgba_geta(c)) * opacity / 255;
	 
	 *dst_address = _rgba(r, g, b, a);
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

static void ink_hline16_blur(int x1, int y, int x2, IToolLoop* loop)
{
  int c, v, a;
  int opacity = loop->getOpacity();
  TiledMode tiled = loop->getTiledMode();
  Image* src = loop->getSrcImage();
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint16 *src_address2;

  DEFINE_INK_PROCESSING_SRCDST
    (GrayscaleTraits,
     {
       c = 0;
       v = a = 0;

       GET_MATRIX_DATA
	 (ase_uint16, src, src_address2,
	  3, 3, 1, 1, tiled,
	  color = *src_address2;
	  if (_graya_geta(color) > 0) {
	    v += _graya_getv(color);
	    a += _graya_geta(color);
	  }
	  c++;
	  );

       if (c > 0) {
	 v /= c;
	 a /= 9;

	 c = *src_address;
	 v = _graya_getv(c) + (v-_graya_getv(c)) * opacity / 255;
	 a = _graya_geta(c) + (a-_graya_geta(c)) * opacity / 255;
	 
	 *dst_address = _graya(v, a);
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

static void ink_hline8_blur(int x1, int y, int x2, IToolLoop* loop)
{
  Palette *pal = get_current_palette();
  RgbMap* rgbmap = loop->getSprite()->getRgbMap();
  int c, r, g, b, a;
  int opacity = loop->getOpacity();
  TiledMode tiled = loop->getTiledMode();
  Image* src = loop->getSrcImage();
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint8 *src_address2;
  
  DEFINE_INK_PROCESSING_SRCDST
    (IndexedTraits,
     {
       c = 0;
       r = g = b = a = 0;

       GET_MATRIX_DATA
	 (ase_uint8, src, src_address2,
	  3, 3, 1, 1, tiled,

	  color = *src_address2;
	  a += (color == 0 ? 0: 255);

	  color = pal->getEntry(color);
	  r += _rgba_getr(color);
	  g += _rgba_getg(color);
	  b += _rgba_getb(color);
	  c++;
	  );

       if (c > 0 && a/9 >= 128) {
	 r /= c;
	 g /= c;
	 b /= c;

	 c = pal->getEntry(*src_address);
	 r = _rgba_getr(c) + (r-_rgba_getr(c)) * opacity / 255;
	 g = _rgba_getg(c) + (g-_rgba_getg(c)) * opacity / 255;
	 b = _rgba_getb(c) + (b-_rgba_getb(c)) * opacity / 255;

	 *dst_address = rgbmap->mapColor(r, g, b);
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

//////////////////////////////////////////////////////////////////////
// Replace Ink
//////////////////////////////////////////////////////////////////////

static void ink_hline32_replace(int x1, int y, int x2, IToolLoop* loop)
{
  ase_uint32 color1 = loop->getPrimaryColor();
  ase_uint32 color2 = loop->getSecondaryColor();
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (RgbTraits,
     if (*src_address == color1) {
       *dst_address = _rgba_blend_normal(*src_address, color2, opacity);
     });
}  

static void ink_hline16_replace(int x1, int y, int x2, IToolLoop* loop)
{
  int color1 = loop->getPrimaryColor();
  int color2 = loop->getSecondaryColor();
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (GrayscaleTraits,
     if (*src_address == color1) {
       *dst_address = _graya_blend_normal(*src_address, color2, opacity);
     });
}  

static void ink_hline8_replace(int x1, int y, int x2, IToolLoop* loop)
{
  int color1 = loop->getPrimaryColor();
  Palette *pal = get_current_palette();
  RgbMap* rgbmap = loop->getSprite()->getRgbMap();
  ase_uint32 c;
  ase_uint32 tc = pal->getEntry(loop->getSecondaryColor());
  int opacity = loop->getOpacity();

  DEFINE_INK_PROCESSING_SRCDST
    (IndexedTraits,
     if (*src_address == color1) {
       c = _rgba_blend_normal(pal->getEntry(*src_address), tc, opacity);
       *dst_address = rgbmap->mapColor(_rgba_getr(c),
				       _rgba_getg(c),
				       _rgba_getb(c));
     });
}

//////////////////////////////////////////////////////////////////////
// Jumble Ink
//////////////////////////////////////////////////////////////////////

#define JUMBLE_XY_IN_UV()						\
  u = x + (rand() % 3)-1 - speed.x;					\
  v = y + (rand() % 3)-1 - speed.y;					\
  									\
  if (tiled & TILED_X_AXIS) {						\
    if (u < 0)								\
      u = loop->getSrcImage()->w - (-(u+1) % loop->getSrcImage()->w) - 1; \
    else if (u >= loop->getSrcImage()->w)				\
      u %= loop->getSrcImage()->w;					\
  }									\
  else {								\
    u = MID(0, u, loop->getSrcImage()->w-1);				\
  }									\
									\
  if (tiled & TILED_Y_AXIS) {						\
    if (v < 0)								\
      v = loop->getSrcImage()->h - (-(v+1) % loop->getSrcImage()->h) - 1; \
    else if (v >= loop->getSrcImage()->h)				\
      v %= loop->getSrcImage()->h;					\
  }									\
  else {								\
    v = MID(0, v, loop->getSrcImage()->h-1);				\
  }									\
  color = image_getpixel(loop->getSrcImage(), u, v);

static void ink_hline32_jumble(int x1, int y, int x2, IToolLoop* loop)
{
  int opacity = loop->getOpacity();
  Point speed(loop->getSpeed() / 4);
  TiledMode tiled = loop->getTiledMode();
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (RgbTraits,
     {
       JUMBLE_XY_IN_UV();
       *dst_address = _rgba_blend_MERGE(*src_address, color, opacity);
     }
     );
}  

static void ink_hline16_jumble(int x1, int y, int x2, IToolLoop* loop)
{
  int opacity = loop->getOpacity();
  Point speed(loop->getSpeed() / 4);
  TiledMode tiled = loop->getTiledMode();
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (GrayscaleTraits,
     {
       JUMBLE_XY_IN_UV();
       *dst_address = _graya_blend_MERGE(*src_address, color, opacity);
     }
     );
}  

static void ink_hline8_jumble(int x1, int y, int x2, IToolLoop* loop)
{
  const Palette *pal = get_current_palette();
  const RgbMap* rgbmap = loop->getSprite()->getRgbMap();
  ase_uint32 c, tc;
  int opacity = loop->getOpacity();
  Point speed(loop->getSpeed() / 4);
  TiledMode tiled = loop->getTiledMode();
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (IndexedTraits,
     {
       JUMBLE_XY_IN_UV();

       tc = color != 0 ? pal->getEntry(color): 0;
       c = _rgba_blend_MERGE(*src_address != 0 ? pal->getEntry(*src_address): 0,
			     tc, opacity);

       if (_rgba_geta(c) >= 128)
	 *dst_address = rgbmap->mapColor(_rgba_getr(c),
					 _rgba_getg(c),
					 _rgba_getb(c));
       else
	 *dst_address = 0;
     }
     );
}  

//////////////////////////////////////////////////////////////////////

enum {
  INK_OPAQUE,
  INK_TRANSPARENT,
  INK_BLUR,
  INK_REPLACE,
  INK_JUMBLE,
  MAX_INKS
};

static AlgoHLine ink_processing[][3] =
{
#define DEF_INK(name)			\
  { (AlgoHLine)ink_hline32_##name,	\
    (AlgoHLine)ink_hline16_##name,	\
    (AlgoHLine)ink_hline8_##name }

  DEF_INK(opaque),
  DEF_INK(transparent),
  DEF_INK(blur),
  DEF_INK(replace),
  DEF_INK(jumble)
};
