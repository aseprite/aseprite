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
 *      Asm routines for software color conversion.
 *      Suggestions to make it faster are welcome :)
 *
 *      By Isaac Cruz.
 *
 *      24-bit color support and non MMX routines by Eric Botcazou.
 *
 *      Support for rectangles of any width, 8-bit destination color
 *      and cross-conversion between 15-bit and 16-bit colors,
 *      additional MMX and color copy routines by Robert J. Ohannessian.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"


int *_colorconv_indexed_palette = NULL;    /* for conversion from 8-bit */
int *_colorconv_rgb_scale_5x35 = NULL;     /* for conversion from 15/16-bit */
unsigned char *_colorconv_rgb_map = NULL;  /* for conversion from 8/12-bit to 8-bit */

static int indexed_palette_depth;          /* target depth of the indexed palette */
static int indexed_palette_size;           /* size of the indexed palette */



/* build_rgb_scale_5235_table:
 *  Builds pre-calculated tables for 15-bit to truecolor conversion.
 */
static void build_rgb_scale_5235_table(int to_depth)
{
   int i, color, red, green, blue;

   if (to_depth == 24)
      /* 6 contiguous 256-entry tables (6k) */
      _colorconv_rgb_scale_5x35 = _AL_MALLOC_ATOMIC(sizeof(int)*1536);
   else if (to_depth == 32)
      /* 2 contiguous 256-entry tables (2k) */
      _colorconv_rgb_scale_5x35 = _AL_MALLOC_ATOMIC(sizeof(int)*512);

   /* 1st table: r5g2 to r8g8b0 */ 
   for (i=0; i<128; i++) {
      red = _rgb_scale_5[i>>2];
      green=((i&3)<<6)+((i&3)<<1);

      color = (red<<16) | (green<<8);
      _colorconv_rgb_scale_5x35[i] = color;

      if (to_depth == 24) {
         _colorconv_rgb_scale_5x35[ 512+i] = (color>>8)+((color&0xff)<<24);
         _colorconv_rgb_scale_5x35[1024+i] = (color>>16)+((color&0xffff)<<16);
      }
   }

   /* 2nd table: g3b5 to r0g8b8 */
   for (i=0; i<256; i++) {
      blue = _rgb_scale_5[i&0x1f];
      green=(i>>5)<<3;

      if (green == 0x38)
          green++;

      color = (green<<8) | blue;
      _colorconv_rgb_scale_5x35[256+i] = color;

      if (to_depth == 24) {
         _colorconv_rgb_scale_5x35[ 512+256+i] = (color>>8)+((color&0xff)<<24);
         _colorconv_rgb_scale_5x35[1024+256+i] = (color>>16)+((color&0xffff)<<16);
      }
   }
}



/* build_rgb_scale_5335_table:
 *  Builds pre-calculated tables for 16-bit to truecolor conversion.
 */
static void build_rgb_scale_5335_table(int to_depth)
{
   int i, color, red, green, blue;

   if (to_depth == 24)
      /* 6 contiguous 256-entry tables (6k) */
      _colorconv_rgb_scale_5x35 = _AL_MALLOC_ATOMIC(sizeof(int)*1536);
   else if (to_depth == 32)
      /* 2 contiguous 256-entry tables (2k) */
      _colorconv_rgb_scale_5x35 = _AL_MALLOC_ATOMIC(sizeof(int)*512);

   /* 1st table: r5g3 to r8g8b0 */ 
   for (i=0; i<256; i++) {
      red = _rgb_scale_5[i>>3];
      green=(i&7)<<5;

      if (green >= 68)
           green++;

       if (green >= 160)
           green++;

      color = (red<<16) | (green<<8);
      _colorconv_rgb_scale_5x35[i] = color;

      if (to_depth == 24) {
         _colorconv_rgb_scale_5x35[ 512+i] = (color>>8)+((color&0xff)<<24);
         _colorconv_rgb_scale_5x35[1024+i] = (color>>16)+((color&0xffff)<<16);
      }
   }

   /* 2nd table: g3b5 to r0g8b8 */
   for (i=0; i<256; i++) {
      blue = _rgb_scale_5[i&0x1f];
      green=(i>>5)<<2;

      if (green == 0x1c)
          green++;

      color = (green<<8) | blue;
      _colorconv_rgb_scale_5x35[256+i] = color;

      if (to_depth == 24) {
         _colorconv_rgb_scale_5x35[ 512+256+i] = (color>>8)+((color&0xff)<<24);
         _colorconv_rgb_scale_5x35[1024+256+i] = (color>>16)+((color&0xffff)<<16);
      }
   }
}



/* create_indexed_palette:
 *  Reserves storage for the 8-bit palette.
 */
static void create_indexed_palette(int to_depth)
{
   switch (to_depth) {

      case 15:
      case 16:
         indexed_palette_size = PAL_SIZE*2;
         break;

      case 24:
         indexed_palette_size = PAL_SIZE*4;
         break;

      case 32:
         indexed_palette_size = PAL_SIZE;
         break;
   }

   indexed_palette_depth = to_depth;
   _colorconv_indexed_palette = _AL_MALLOC_ATOMIC(sizeof(int) * indexed_palette_size);
}



/* _set_colorconv_palette:
 *  Updates 8-bit palette entries.
 */
void _set_colorconv_palette(AL_CONST struct RGB *p, int from, int to)
{
   int n, color;

   if (!indexed_palette_size)
      return;

   for (n = from; n <= to; n++) {
      color = makecol_depth(indexed_palette_depth,
                            (p[n].r << 2) | ((p[n].r & 0x30) >> 4),
                            (p[n].g << 2) | ((p[n].g & 0x30) >> 4),
                            (p[n].b << 2) | ((p[n].b & 0x30) >> 4));

      _colorconv_indexed_palette[n] = color;

      if ((indexed_palette_depth == 15) || (indexed_palette_depth == 16)) {
         /* 2 pre-calculated shift tables (2k) */
         _colorconv_indexed_palette[PAL_SIZE+n] = color<<16; 
      }
      else if (indexed_palette_depth == 24) {
         /* 4 pre-calculated shift tables (4k) */
         _colorconv_indexed_palette[PAL_SIZE+n]   = (color>>8)+((color&0xff)<<24);
         _colorconv_indexed_palette[PAL_SIZE*2+n] = (color>>16)+((color&0xffff)<<16);
         _colorconv_indexed_palette[PAL_SIZE*3+n] = color<<8;
      }
   }
}



/* create_rgb_map:
 *  Reserves storage for the rgb map to 8-bit.
 */
static void create_rgb_map(int from_depth)
{
   int rgb_map_size = 0;

   switch (from_depth) {
      case 8:
         rgb_map_size = 256;  /* 8-bit */
         break;

      case 15:
      case 16:
      case 24:
      case 32:
         rgb_map_size = 4096;  /* 12-bit */
         break;
   }

   _colorconv_rgb_map = _AL_MALLOC_ATOMIC(sizeof(int) * rgb_map_size);
}



/* _get_colorconv_map:
 *  Retrieves a handle to the rgb map.
 */
unsigned char *_get_colorconv_map(void)
{
   return _colorconv_rgb_map;
}



/* _get_colorconv_blitter:
 *  Returns the blitter function matching the specified depths.
 */
COLORCONV_BLITTER_FUNC *_get_colorconv_blitter(int from_depth, int to_depth)
{
   switch (from_depth) {

#ifdef ALLEGRO_COLOR8
      case 8:
         switch (to_depth) {

            case 8:
               create_rgb_map(8);
               return &_colorconv_blit_8_to_8;

            case 15:
               create_indexed_palette(15);
               return &_colorconv_blit_8_to_15;

            case 16:
               create_indexed_palette(16);
               return &_colorconv_blit_8_to_16;

            case 24:
               create_indexed_palette(24);
               return &_colorconv_blit_8_to_24;

            case 32:
               create_indexed_palette(32);
               return &_colorconv_blit_8_to_32;
         }
         break;
#endif

#ifdef ALLEGRO_COLOR16
      case 15:
         switch (to_depth) {

            case 8:
               create_rgb_map(15);
               return &_colorconv_blit_15_to_8;

            case 15:
#ifndef ALLEGRO_NO_COLORCOPY
               return &_colorcopy_blit_15_to_15;
#else
               return NULL;
#endif

            case 16:
               return &_colorconv_blit_15_to_16;

            case 24:
               build_rgb_scale_5235_table(24);
               return &_colorconv_blit_15_to_24;

            case 32:
               build_rgb_scale_5235_table(32);
               return &_colorconv_blit_15_to_32;
         }
         break;

      case 16:
         switch (to_depth) {

            case 8:
               create_rgb_map(16);
               return &_colorconv_blit_16_to_8;

            case 15:
               return &_colorconv_blit_16_to_15;

            case 16:
#ifndef ALLEGRO_NO_COLORCOPY
               return &_colorcopy_blit_16_to_16;
#else
               return NULL;
#endif

            case 24:
               build_rgb_scale_5335_table(24);
               return &_colorconv_blit_16_to_24;

            case 32:
               build_rgb_scale_5335_table(32);
               return &_colorconv_blit_16_to_32;
         }
         break;
#endif

#ifdef ALLEGRO_COLOR24
      case 24:
         switch (to_depth) {

            case 8:
               create_rgb_map(24);
               return &_colorconv_blit_24_to_8;

            case 15:
               return &_colorconv_blit_24_to_15;

            case 16:
               return &_colorconv_blit_24_to_16;

            case 24:
#ifndef ALLEGRO_NO_COLORCOPY
               return &_colorcopy_blit_24_to_24;
#else
               return NULL;
#endif

            case 32:
               return &_colorconv_blit_24_to_32;
         }
         break;
#endif

#ifdef ALLEGRO_COLOR32
      case 32:
         switch (to_depth) {

            case 8:
               create_rgb_map(32);
               return &_colorconv_blit_32_to_8;

            case 15:
               return &_colorconv_blit_32_to_15;

            case 16:
               return &_colorconv_blit_32_to_16;

            case 24:
               return &_colorconv_blit_32_to_24;

            case 32:
#ifndef ALLEGRO_NO_COLORCOPY
               return &_colorcopy_blit_32_to_32;
#else
               return NULL;
#endif
         }
         break;
#endif
   }

   return NULL;
}



/* _release_colorconv_blitter:
 *  Frees previously allocated resources.
 */
void _release_colorconv_blitter(COLORCONV_BLITTER_FUNC *blitter)
{
   /* destroy the 8-bit palette */
   if (_colorconv_indexed_palette) {
      _AL_FREE(_colorconv_indexed_palette);
      _colorconv_indexed_palette = NULL;
      indexed_palette_size = 0;
   }

   /* destroy the shift table */
   if (_colorconv_rgb_scale_5x35) {
      _AL_FREE(_colorconv_rgb_scale_5x35);
      _colorconv_rgb_scale_5x35 = NULL;
   }

   /* destroy the rgb map to 8-bit */
   if (_colorconv_rgb_map) {
      _AL_FREE(_colorconv_rgb_map);
      _colorconv_rgb_map = NULL;
   }
}

