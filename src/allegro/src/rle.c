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
 *      RLE sprite generation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* get_rle_sprite:
 *  Creates a run length encoded sprite based on the specified bitmap.
 *  The returned sprite is likely to be a lot smaller than the original
 *  bitmap, and can be drawn to the screen with draw_rle_sprite().
 *
 *  The compression is done individually for each line of the image.
 *  Format is a series of command bytes, 1-127 marks a run of that many
 *  solid pixels, negative numbers mark a gap of -n pixels, and 0 marks
 *  the end of a line (since zero can't occur anywhere else in the data,
 *  this can be used to find the start of a specified line when clipping).
 *  For truecolor RLE sprites, the data and command bytes are both in the
 *  same format (16 or 32 bits, 24 bpp data is padded to 32 bit aligment), 
 *  and the mask color (bright pink) is used as the EOL marker.
 */
RLE_SPRITE *get_rle_sprite(BITMAP *bitmap)
{
   int depth;
   RLE_SPRITE *s;
   int x, y;
   int run;
   int pix;
   int c;
   ASSERT(bitmap);
   
   depth = bitmap_color_depth(bitmap);

   #define WRITE_TO_SPRITE8(x) {                                             \
      _grow_scratch_mem(c+1);                                                \
      p = (signed char *)_scratch_mem;                                       \
      p[c] = x;                                                              \
      c++;                                                                   \
   }

   #define WRITE_TO_SPRITE16(x) {                                            \
      _grow_scratch_mem((c+1)*sizeof(int16_t));                              \
      p = (int16_t *)_scratch_mem;                                           \
      p[c] = x;                                                              \
      c++;                                                                   \
   }

   #define WRITE_TO_SPRITE32(x) {                                            \
      _grow_scratch_mem((c+1)*sizeof(int32_t));                              \
      p = (int32_t *)_scratch_mem;                                           \
      p[c] = x;                                                              \
      c++;                                                                   \
   }

   /* helper for building an RLE run */
   #define DO_RLE(bits)                                                      \
   {                                                                         \
      for (y=0; y<bitmap->h; y++) {                                          \
	 run = -1;                                                           \
	 for (x=0; x<bitmap->w; x++) {                                       \
	    pix = getpixel(bitmap, x, y) & 0xFFFFFF;                         \
	    if (pix != bitmap->vtable->mask_color) {                         \
	       if ((run >= 0) && (p[run] > 0) && (p[run] < 127))             \
		  p[run]++;                                                  \
	       else {                                                        \
		  run = c;                                                   \
		  WRITE_TO_SPRITE##bits(1);                                  \
	       }                                                             \
	       WRITE_TO_SPRITE##bits(getpixel(bitmap, x, y));                \
	    }                                                                \
	    else {                                                           \
	       if ((run >= 0) && (p[run] < 0) && (p[run] > -128))            \
		  p[run]--;                                                  \
	       else {                                                        \
		  run = c;                                                   \
		  WRITE_TO_SPRITE##bits(-1);                                 \
	       }                                                             \
	    }                                                                \
	 }                                                                   \
	 WRITE_TO_SPRITE##bits(bitmap->vtable->mask_color);                  \
      }                                                                      \
   }

   c = 0;

   switch (depth) {

      #ifdef ALLEGRO_COLOR8

	 case 8:
	    {
	       signed char *p = (signed char *)_scratch_mem;
	       DO_RLE(8);
	    }
	    break;

      #endif

      #ifdef ALLEGRO_COLOR16

	 case 15:
	 case 16:
	    {
	       signed short *p = (signed short *)_scratch_mem;
	       DO_RLE(16);
	       c *= sizeof(short);
	    }
	    break;

      #endif

      #if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

	 case 24:
	 case 32:
	    {
	       int32_t *p = (int32_t *)_scratch_mem;
	       DO_RLE(32);
	       c *= sizeof(int32_t);
	    }
	    break;

      #endif
   }

   s = _AL_MALLOC(sizeof(RLE_SPRITE) + c);

   if (s) {
      s->w = bitmap->w;
      s->h = bitmap->h;
      s->color_depth = depth;
      s->size = c;
      memcpy(s->dat, _scratch_mem, c);
   }

   return s;
}



/* destroy_rle_sprite:
 *  Destroys an RLE sprite structure returned by get_rle_sprite().
 */
void destroy_rle_sprite(RLE_SPRITE *sprite)
{
   if (sprite)
      _AL_FREE(sprite);
}


