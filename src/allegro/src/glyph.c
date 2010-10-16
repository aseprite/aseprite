/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Monochrome character drawing routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* helper macro for drawing glyphs in each color depth */
#define DRAW_GLYPH(bits, size)                                               \
{                                                                            \
   AL_CONST unsigned char *data = glyph->dat;                                \
   unsigned long addr;                                                       \
   int w = glyph->w;                                                         \
   int h = glyph->h;                                                         \
   int stride = (w+7)/8;                                                     \
   int lgap = 0;                                                             \
   int d, i, j;                                                              \
									     \
   if (bmp->clip) {                                                          \
      /* clip the top */                                                     \
      if (y < bmp->ct) {                                                     \
	 d = bmp->ct - y;                                                    \
									     \
	 h -= d;                                                             \
	 if (h <= 0)                                                         \
	    return;                                                          \
									     \
	 data += d*stride;                                                   \
	 y = bmp->ct;                                                        \
      }                                                                      \
									     \
      /* clip the bottom */                                                  \
      if (y+h >= bmp->cb) {                                                  \
	 h = bmp->cb - y;                                                    \
	 if (h <= 0)                                                         \
	    return;                                                          \
      }                                                                      \
									     \
      /* clip the left */                                                    \
      if (x < bmp->cl) {                                                     \
	 d = bmp->cl - x;                                                    \
									     \
	 w -= d;                                                             \
	 if (w <= 0)                                                         \
	    return;                                                          \
									     \
	 data += d/8;                                                        \
	 lgap = d&7;                                                         \
	 x = bmp->cl;                                                        \
      }                                                                      \
									     \
      /* clip the right */                                                   \
      if (x+w >= bmp->cr) {                                                  \
	 w = bmp->cr - x;                                                    \
	 if (w <= 0)                                                         \
	    return;                                                          \
      }                                                                      \
   }                                                                         \
									     \
   stride -= (lgap+w+7)/8;                                                   \
									     \
   /* draw it */                                                             \
   bmp_select(bmp);                                                          \
									     \
   while (h--) {                                                             \
      addr = bmp_write_line(bmp, y++) + x*size;                              \
									     \
      j = 0;                                                                 \
      i = 0x80 >> lgap;                                                      \
      d = *(data++);                                                         \
									     \
      if (bg >= 0) {                                                         \
	 /* opaque mode drawing loop */                                      \
	 for (;;) {                                                          \
	    if (d & i)                                                       \
	       bmp_write##bits(addr, color);                                 \
	    else                                                             \
	       bmp_write##bits(addr, bg);                                    \
									     \
	    j++;                                                             \
	    if (j == w)                                                      \
	       break;                                                        \
									     \
	    i >>= 1;                                                         \
	    if (!i) {                                                        \
	       i = 0x80;                                                     \
	       d = *(data++);                                                \
	    }                                                                \
									     \
	    addr += size;                                                    \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 /* masked mode drawing loop */                                      \
	 for (;;) {                                                          \
	    if (d & i)                                                       \
	       bmp_write##bits(addr, color);                                 \
									     \
	    j++;                                                             \
	    if (j == w)                                                      \
	       break;                                                        \
									     \
	    i >>= 1;                                                         \
	    if (!i) {                                                        \
	       i = 0x80;                                                     \
	       d = *(data++);                                                \
	    }                                                                \
									     \
	    addr += size;                                                    \
	 }                                                                   \
      }                                                                      \
									     \
      data += stride;                                                        \
   }                                                                         \
									     \
   bmp_unwrite_line(bmp);                                                    \
}



#ifdef ALLEGRO_COLOR8

/* _linear_draw_glyph8:
 *  Draws a glyph onto an 8 bit bitmap.
 */
void _linear_draw_glyph8(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   DRAW_GLYPH(8, 1);
}

#endif



#ifdef ALLEGRO_COLOR16

/* _linear_draw_glyph16:
 *  Draws a glyph onto a 16 bit bitmap.
 */
void _linear_draw_glyph16(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   DRAW_GLYPH(16, sizeof(int16_t));
}

#endif



#ifdef ALLEGRO_COLOR24

/* _linear_draw_glyph24:
 *  Draws a glyph onto a 24 bit bitmap.
 */
void _linear_draw_glyph24(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   DRAW_GLYPH(24, 3);
}

#endif



#ifdef ALLEGRO_COLOR32

/* _linear_draw_glyph32:
 *  Draws a glyph onto a 32 bit bitmap.
 */
void _linear_draw_glyph32(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   DRAW_GLYPH(32, sizeof(int32_t));
}

#endif

