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
 *      C routines for software color conversion.
 *
 *      By Angelo Mottola, based on x86 asm versions by Isaac Cruz,
 *      Eric Botcazou and Robert J. Ohannessian.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"


extern int *_colorconv_indexed_palette;    /* for conversion from 8-bit */
extern int *_colorconv_rgb_scale_5x35;     /* for conversion from 15/16-bit */
extern unsigned char *_colorconv_rgb_map;  /* for conversion from 8/12-bit to 8-bit */


#ifdef ALLEGRO_NO_ASM

#ifdef ALLEGRO_COLOR8


void _colorconv_blit_8_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int dest_data;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - width;
   dest_feed = dest_rect->pitch - width;
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
         src_data = *(unsigned int *)src;
	 dest_data = _colorconv_rgb_map[src_data & 0xff];
	 dest_data |= (_colorconv_rgb_map[(src_data >> 8) & 0xff] << 8);
	 dest_data |= (_colorconv_rgb_map[(src_data >> 16) & 0xff] << 16);
	 dest_data |= (_colorconv_rgb_map[src_data >> 24] << 24);
	 *(unsigned int *)dest = dest_data;
	 src += 4;
	 dest += 4;
      }
      if (width & 0x2) {
         src_data = *(unsigned short *)src;
	 dest_data = _colorconv_rgb_map[src_data & 0xff];
	 dest_data |= (_colorconv_rgb_map[src_data >> 8] << 8);
	 *(unsigned short *)dest = (unsigned short)dest_data;
	 src += 2;
	 dest += 2;
      }
      if (width & 0x1) {
         src_data = *(unsigned char *)src;
	 dest_data = _colorconv_rgb_map[src_data];
	 *(unsigned char *)dest = (unsigned char)dest_data;
	 src++;
	 dest++;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_8_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   _colorconv_blit_8_to_16(src_rect, dest_rect);
}



void _colorconv_blit_8_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int dest_data;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - width;
   dest_feed = dest_rect->pitch - (width << 1);
   src = src_rect->data;
   dest = dest_rect->data;

#ifdef ALLEGRO_ARM

   /* ARM requires strict alignment for memory access. So this ARM branch of code 
    * takes it into account. Combining data into 32-bit values when writing to
    * memory really adds about 20% extra performance on Nokia 770. An interesting 
    * thing is that this branch of code is not only correct, but also actually 
    * a bit faster than the other one.
    *
    * TODO: Check performance of this code path on other architectures.  If it
    * is reasonably similar or better to the non-aligned code path then we can
    * remove this #ifdef.
    */
   #ifdef ALLEGRO_LITTLE_ENDIAN
      #define FILL_DEST_DATA() \
	 dest_data = _colorconv_indexed_palette[src[0]]; \
	 dest_data |= _colorconv_indexed_palette[256 + src[1]]; \
	 src += 2;
   #else
      #define FILL_DEST_DATA() \
	 dest_data = _colorconv_indexed_palette[256 + src[0]]; \
	 dest_data |= _colorconv_indexed_palette[src[1]]; \
	 src += 2;
   #endif

   if (width <= 0)
      return;

   y = src_rect->height;
   if (width & 0x1) {
      width >>= 1;
      while (--y >= 0) {
         if ((int)dest & 0x3) {
            *(unsigned short *)(dest) = _colorconv_indexed_palette[*src++]; dest += 2;
            x = width; while (--x >= 0) { FILL_DEST_DATA(); *(unsigned int *)dest = dest_data; dest += 4; }
         } else {
            x = width; while (--x >= 0) { FILL_DEST_DATA(); *(unsigned int *)dest = dest_data; dest += 4; }
            *(unsigned short *)(dest) = _colorconv_indexed_palette[*src++]; dest += 2;
         }
         src += src_feed;
         dest += dest_feed;
      }
   }
   else {
      width >>= 1;
      while (--y >= 0) {
         if ((int)dest & 0x3) {
            *(unsigned short *)(dest) = _colorconv_indexed_palette[*src++]; dest += 2;
            x = width; while (--x > 0) { FILL_DEST_DATA(); *(unsigned int *)dest = dest_data; dest += 4; }
            *(unsigned short *)(dest) = _colorconv_indexed_palette[*src++]; dest += 2;
         } else {
            x = width; while (--x >= 0) { FILL_DEST_DATA(); *(unsigned int *)dest = dest_data; dest += 4; }
         }
         src += src_feed;
         dest += dest_feed;
      }
   }

#else /* !ALLEGRO_ARM */

   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
         src_data = *(unsigned int *)src;
	 #ifdef ALLEGRO_LITTLE_ENDIAN
	    dest_data = _colorconv_indexed_palette[src_data & 0xff];
	    dest_data |= _colorconv_indexed_palette[256 + ((src_data >> 8) & 0xff)];
	    *(unsigned int *)dest = dest_data;
	    dest_data = _colorconv_indexed_palette[(src_data >> 16) & 0xff];
	    dest_data |= _colorconv_indexed_palette[256 + (src_data >> 24)];
	    *(unsigned int *)(dest + 4) = dest_data;
	 #else
	    dest_data = _colorconv_indexed_palette[256 + (src_data >> 24)];
	    dest_data |= _colorconv_indexed_palette[((src_data >> 16) & 0xff)];
	    *(unsigned int *)dest = dest_data;
	    dest_data = _colorconv_indexed_palette[256 + ((src_data >> 8) & 0xff)];
	    dest_data |= _colorconv_indexed_palette[(src_data & 0xff)];
	    *(unsigned int *)(dest + 4) = dest_data;
	 #endif
	 src += 4;
	 dest += 8;
      }
      if (width & 0x2) {
         src_data = *(unsigned short *)src;
	 #ifdef ALLEGRO_LITTLE_ENDIAN
	    dest_data = _colorconv_indexed_palette[src_data & 0xff];
	    dest_data |= _colorconv_indexed_palette[256 + (src_data >> 8)];
	    *(unsigned int *)dest = dest_data;
	 #else
	    dest_data = _colorconv_indexed_palette[src_data >> 8];
	    dest_data |= _colorconv_indexed_palette[256 + (src_data & 0xff)];
	    *(unsigned int *)dest = dest_data;
	 #endif
	 src += 2;
	 dest += 4;
      }
      if (width & 0x1) {
         src_data = *(unsigned char *)src;
	 dest_data = _colorconv_indexed_palette[src_data];
	 *(unsigned short *)dest = (unsigned short)dest_data;
	 src++;
	 dest += 2;
      }
      src += src_feed;
      dest += dest_feed;
   }

#endif	 /* !ALLEGRO_ARM */
}



void _colorconv_blit_8_to_24(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int temp1, temp2, temp3, temp4;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - width;
   dest_feed = dest_rect->pitch - (width * 3);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
         src_data = *(unsigned int *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 temp1 = _colorconv_indexed_palette[src_data & 0xff];
	 temp2 = _colorconv_indexed_palette[256 + ((src_data >> 8) & 0xff)];
	 temp3 = _colorconv_indexed_palette[512 + ((src_data >> 16) & 0xff)];
	 temp4 = _colorconv_indexed_palette[768 + (src_data >> 24)];
#else
	 temp1 = _colorconv_indexed_palette[768 + (src_data >> 24)];
	 temp2 = _colorconv_indexed_palette[512 + ((src_data >> 16) & 0xff)];
	 temp3 = _colorconv_indexed_palette[256 + ((src_data >> 8) & 0xff)];
	 temp4 = _colorconv_indexed_palette[src_data & 0xff];
#endif
	 *(unsigned int *)dest = temp1 | (temp2 & 0xff000000);
	 *(unsigned int *)(dest + 4) = (temp2 & 0xffff) | (temp3 & 0xffff0000);
	 *(unsigned int *)(dest + 8) = temp4 | (temp3 & 0xff);
	 src += 4;
	 dest += 12;
      }
      if (width & 0x2) {
         src_data = *(unsigned short *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 temp1 = _colorconv_indexed_palette[src_data & 0xff];
	 temp2 = _colorconv_indexed_palette[src_data >> 8];
#else
	 temp1 = _colorconv_indexed_palette[src_data >> 8];
	 temp2 = _colorconv_indexed_palette[src_data & 0xff];
#endif
	 *(unsigned int *)dest = temp1;
	 *(unsigned short *)(dest + 3) = (unsigned short)(temp2 & 0xffff);
	 *(unsigned char *)(dest + 5) = (unsigned char)(temp2 >> 16);
	 src += 2;
	 dest += 6;
      }
      if (width & 0x1) {
         src_data = *(unsigned char *)src;
	 temp1 = _colorconv_indexed_palette[src_data];
	 *(unsigned short *)dest = (unsigned short)(temp1 & 0xffff);
	 *(unsigned char *)(dest + 2) = (unsigned char)(temp1 >> 16);
	 src++;
	 dest += 3;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_8_to_32(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int dest_data;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - width;
   dest_feed = dest_rect->pitch - (width << 2);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
         src_data = *(unsigned int *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 dest_data = _colorconv_indexed_palette[src_data & 0xff];
	 *(unsigned int *)(dest) = dest_data;
	 dest_data = _colorconv_indexed_palette[(src_data >> 8) & 0xff];
	 *(unsigned int *)(dest + 4) = dest_data;
	 dest_data = _colorconv_indexed_palette[(src_data >> 16) & 0xff];
	 *(unsigned int *)(dest + 8) = dest_data;
	 dest_data = _colorconv_indexed_palette[(src_data >> 24) & 0xff];
	 *(unsigned int *)(dest + 12) = dest_data;
#else
	 dest_data = _colorconv_indexed_palette[(src_data >> 24) & 0xff];
	 *(unsigned int *)(dest) = dest_data;
	 dest_data = _colorconv_indexed_palette[(src_data >> 16) & 0xff];
	 *(unsigned int *)(dest + 4) = dest_data;
	 dest_data = _colorconv_indexed_palette[(src_data >> 8) & 0xff];
	 *(unsigned int *)(dest + 8) = dest_data;
	 dest_data = _colorconv_indexed_palette[src_data & 0xff];
	 *(unsigned int *)(dest + 12) = dest_data;
#endif
	 src += 4;
	 dest += 16;
      }
      if (width & 0x2) {
         src_data = *(unsigned short *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 dest_data = _colorconv_indexed_palette[src_data & 0xff];
	 *(unsigned int *)(dest) = dest_data;
	 dest_data = _colorconv_indexed_palette[src_data >> 8];
	 *(unsigned int *)(dest + 4) = dest_data;
#else
	 dest_data = _colorconv_indexed_palette[src_data >> 8];
	 *(unsigned int *)(dest) = dest_data;
	 dest_data = _colorconv_indexed_palette[src_data & 0xff];
	 *(unsigned int *)(dest + 4) = dest_data;
#endif
	 src += 2;
	 dest += 8;
      }
      if (width & 0x1) {
         src_data = *(unsigned char *)src;
	 *(unsigned int *)dest = _colorconv_indexed_palette[src_data];
	 src++;
	 dest += 4;
      }
      src += src_feed;
      dest += dest_feed;
   }
}


#endif

#ifdef ALLEGRO_COLOR16


void _colorconv_blit_15_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned short dest_data;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - width;
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
         src_data = *(unsigned int *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x03c0) >> 2) | ((src_data & 0x7800) >> 3);
	 dest_data = _colorconv_rgb_map[temp];
	 src_data >>= 16;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x03c0) >> 2) | ((src_data & 0x7800) >> 3);
	 dest_data |= ((unsigned short)_colorconv_rgb_map[temp] << 8);
#else
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x03c0) >> 2) | ((src_data & 0x7800) >> 3);
	 dest_data = ((unsigned short)_colorconv_rgb_map[temp] << 8);
	 src_data >>= 16;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x03c0) >> 2) | ((src_data & 0x7800) >> 3);
	 dest_data |= (unsigned short)_colorconv_rgb_map[temp];
#endif
	 *(unsigned short *)dest = dest_data;
	 src += 4;
	 dest += 2;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x03c0) >> 2) | ((src_data & 0x7800) >> 3);
	 *(unsigned char *)dest = _colorconv_rgb_map[temp];
	 src += 2;
	 dest++;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_15_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - (width << 1);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
         src_data = *(unsigned int *)src;
	 temp = ((src_data & 0x7fe07fe0) << 1) | (src_data & 0x001f001f);
	 *(unsigned int *)dest = (temp | 0x00200020);
	 src += 4;
	 dest += 4;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp = ((src_data & 0x7fe0) << 1) | (src_data & 0x001f);
	 *(unsigned short *)dest = (unsigned short)(temp | 0x0020);
	 src += 2;
	 dest += 2;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_15_to_24(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int temp1, temp2, temp3, temp4;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - (width * 3);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         src_data = *(unsigned int *)src;
	 temp1 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[(src_data >> 8) & 0xff];
         temp2 = _colorconv_rgb_scale_5x35[768 + ((src_data >> 16) & 0xff)] + _colorconv_rgb_scale_5x35[512 + (src_data >> 24)];
	 src_data = *(unsigned int *)(src + 4);
	 temp3 = _colorconv_rgb_scale_5x35[1280 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[1024 + ((src_data >> 8) & 0xff)];
	 temp4 = _colorconv_rgb_scale_5x35[src_data >> 24] + _colorconv_rgb_scale_5x35[256 + ((src_data >> 16) & 0xff)];
#else
         src_data = *(unsigned int *)src;
	 temp4 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[(src_data >> 8) & 0xff];
         temp3 = _colorconv_rgb_scale_5x35[768 + ((src_data >> 16) & 0xff)] + _colorconv_rgb_scale_5x35[512 + (src_data >> 24)];
	 src_data = *(unsigned int *)(src + 4);
	 temp2 = _colorconv_rgb_scale_5x35[1280 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[1024 + ((src_data >> 8) & 0xff)];
	 temp1 = _colorconv_rgb_scale_5x35[src_data >> 24] + _colorconv_rgb_scale_5x35[256 + ((src_data >> 16) & 0xff)];
#endif
	 *(unsigned int *)dest = temp1 | (temp2 & 0xff000000);
	 *(unsigned int *)(dest + 4) = (temp2 & 0xffff) | (temp3 & 0xffff0000);
	 *(unsigned int *)(dest + 8) = (temp3 & 0xff) | (temp4 << 8);
	 src += 8;
	 dest += 12;
      }
      if (width & 0x2) {
         src_data = *(unsigned int *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 temp1 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[(src_data >> 8) & 0xff];
	 temp2 = _colorconv_rgb_scale_5x35[256 + ((src_data >> 16) & 0xff)] + _colorconv_rgb_scale_5x35[src_data >> 24];
#else
	 temp2 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[(src_data >> 8) & 0xff];
	 temp1 = _colorconv_rgb_scale_5x35[256 + ((src_data >> 16) & 0xff)] + _colorconv_rgb_scale_5x35[src_data >> 24];
#endif
	 *(unsigned int *)dest = temp1;
	 *(unsigned short *)(dest + 3) = (unsigned short)(temp2 & 0xffff);
	 *(unsigned char *)(dest + 5) = (unsigned char)(temp2 >> 16);
	 src += 4;
	 dest += 6;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp1 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[src_data >> 8];
	 *(unsigned short *)dest = (unsigned short)(temp1 & 0xffff);
	 *(unsigned char *)(dest + 2) = (unsigned char)(temp1 >> 16);
	 src += 2;
	 dest += 3;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_15_to_32(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int temp1, temp2;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - (width << 2);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
         src_data = *(unsigned int *)src;
	 temp1 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[(src_data >> 8) & 0xff];
         temp2 = _colorconv_rgb_scale_5x35[256 + ((src_data >> 16) & 0xff)] + _colorconv_rgb_scale_5x35[src_data >> 24];
#ifdef ALLEGRO_LITTLE_ENDIAN
	 *(unsigned int *)dest = temp1;
	 *(unsigned int *)(dest + 4) = temp2;
#else
	 *(unsigned int *)dest = temp2;
	 *(unsigned int *)(dest + 4) = temp1;
#endif
	 src += 4;
	 dest += 8;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp1 = _colorconv_rgb_scale_5x35[256 + (src_data & 0xff)] + _colorconv_rgb_scale_5x35[src_data >> 8];
	 *(unsigned int *)dest = temp1;
	 src += 2;
	 dest += 4;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_16_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned short dest_data;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - width;
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         src_data = *(unsigned int *)src;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x0780) >> 3) | ((src_data & 0xf000) >> 4);
	 dest_data = _colorconv_rgb_map[temp];
	 src_data >>= 16;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x0780) >> 3) | ((src_data & 0xf000) >> 4);
	 dest_data |= ((unsigned short)_colorconv_rgb_map[temp] << 8);
	 *(unsigned short *)dest = dest_data;
#else
         src_data = *(unsigned int *)src;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x0780) >> 3) | ((src_data & 0xf000) >> 4);
	 dest_data = ((unsigned short)_colorconv_rgb_map[temp] << 8);
	 src_data >>= 16;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x0780) >> 3) | ((src_data & 0xf000) >> 4);
	 dest_data |= (unsigned short)_colorconv_rgb_map[temp];
	 *(unsigned short *)dest = dest_data;
#endif
	 src += 4;
	 dest += 2;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp = ((src_data & 0x001e) >> 1) | ((src_data & 0x0780) >> 3) | ((src_data & 0xf000) >> 4);
	 *(unsigned char *)dest = _colorconv_rgb_map[temp];
	 src += 2;
	 dest++;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_16_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int src_data;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 1);
   dest_feed = dest_rect->pitch - (width << 1);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
         src_data = *(unsigned int *)src;
	 temp = ((src_data & 0xffc0ffc0) >> 1) | (src_data & 0x001f001f);
	 *(unsigned int *)dest = temp;
	 src += 4;
	 dest += 4;
      }
      if (width & 0x1) {
         src_data = *(unsigned short *)src;
	 temp = ((src_data & 0xffc0) >> 1) | (src_data & 0x001f);
	 *(unsigned short *)dest = (unsigned short)temp;
	 src += 2;
	 dest += 2;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



void _colorconv_blit_16_to_24(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   _colorconv_blit_15_to_24(src_rect, dest_rect);
}



void _colorconv_blit_16_to_32(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   _colorconv_blit_15_to_32(src_rect, dest_rect);
}


#endif

#if (defined ALLEGRO_COLOR24 || defined ALLEGRO_COLOR32)


static void colorconv_blit_true_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width * bpp);
   dest_feed = dest_rect->pitch - width;
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width; x; x--) {
         temp = ((*(unsigned char *)src) >> 4) | ((*(unsigned char *)(src + 1)) & 0xf0) | (((*(unsigned char *)(src + 2)) & 0xf0) << 4);
	 *(unsigned char *)dest = _colorconv_rgb_map[temp];
	 src += bpp;
	 dest++;
      }
      src += src_feed;
      dest += dest_feed;
   }
}


static void colorconv_blit_true_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width * bpp);
   dest_feed = dest_rect->pitch - (width << 1);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         temp = ((*(unsigned char *)(src + bpp)) >> 3) |
	    (((*(unsigned char *)(src + bpp + 1)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + bpp + 2)) << 7) & 0x7c00);
	 temp <<= 16;
         temp |= ((*(unsigned char *)(src)) >> 3) |
	    (((*(unsigned char *)(src + 1)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + 2)) << 7) & 0x7c00);
#else
         temp = ((*(unsigned char *)(src + bpp - 1)) >> 3) |
	    (((*(unsigned char *)(src + bpp - 2)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + bpp - 3)) << 7) & 0x7c00);
	 temp <<= 16;
         temp |= ((*(unsigned char *)(src + bpp + bpp - 1)) >> 3) |
	    (((*(unsigned char *)(src + bpp + bpp - 2)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + bpp + bpp - 3)) << 7) & 0x7c00);
#endif
	 *(unsigned int *)dest = temp;
	 src += (bpp << 1);
	 dest += 4;
      }
      if (width & 0x1) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         temp = ((*(unsigned char *)(src)) >> 3) |
	    (((*(unsigned char *)(src + 1)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + 2)) << 7) & 0x7c00);
#else
         temp = ((*(unsigned char *)(src + bpp - 1)) >> 3) |
	    (((*(unsigned char *)(src + bpp - 2)) << 2) & 0x03e0) |
	    (((*(unsigned char *)(src + bpp - 3)) << 7) & 0x7c00);
#endif
         *(unsigned short *)dest = (unsigned short)temp;
	 src += bpp;
	 dest += 2;
      }
      src += src_feed;
      dest += dest_feed;
   }
}



static void colorconv_blit_true_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width * bpp);
   dest_feed = dest_rect->pitch - (width << 1);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 1; x; x--) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         temp = ((*(unsigned char *)(src + bpp)) >> 3) |
	    (((*(unsigned char *)(src + bpp + 1)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + bpp + 2)) << 8) & 0xf800);
	 temp <<= 16;
         temp |= ((*(unsigned char *)(src)) >> 3) |
	    (((*(unsigned char *)(src + 1)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + 2)) << 8) & 0xf800);
#else
         temp = ((*(unsigned char *)(src + 3)) >> 3) |
	    (((*(unsigned char *)(src + 2)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + 1)) << 8) & 0xf800);
	 temp <<= 16;
         temp |= ((*(unsigned char *)(src + bpp + 3)) >> 3) |
	    (((*(unsigned char *)(src + bpp + 2)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + bpp + 1)) << 8) & 0xf800);
#endif
	 *(unsigned int *)dest = temp;
	 src += (bpp << 1);
	 dest += 4;
      }
      if (width & 0x1) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         temp = ((*(unsigned char *)(src)) >> 3) |
	    (((*(unsigned char *)(src + 1)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + 2)) << 8) & 0xf800);
#else
         temp = ((*(unsigned char *)(src + 3)) >> 3) |
	    (((*(unsigned char *)(src + 2)) << 3) & 0x07e0) |
	    (((*(unsigned char *)(src + 1)) << 8) & 0xf800);
#endif
         *(unsigned short *)dest = (unsigned short)temp;
	 src += bpp;
	 dest += 2;
      }
      src += src_feed;
      dest += dest_feed;
   }
}


#endif

#ifdef ALLEGRO_COLOR24


void _colorconv_blit_24_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_8(src_rect, dest_rect, 3);
}



void _colorconv_blit_24_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_15(src_rect, dest_rect, 3);
}



void _colorconv_blit_24_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_16(src_rect, dest_rect, 3);
}



void _colorconv_blit_24_to_32(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width * 3);
   dest_feed = dest_rect->pitch - (width << 2);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width; x; x--) {
#ifdef ALLEGRO_LITTLE_ENDIAN
         temp = (*(unsigned char *)src) | ((*(unsigned char *)(src + 1)) << 8) | ((*(unsigned char *)(src + 2)) << 16);
#else
         temp = (*(unsigned char *)(src + 2)) | ((*(unsigned char *)(src + 1)) << 8) | ((*(unsigned char *)(src)) << 16);
#endif
	 *(unsigned int *)dest = temp;
	 src += 3;
	 dest += 4;
      }
      src += src_feed;
      dest += dest_feed;
   }
}


#endif

#ifdef ALLEGRO_COLOR32


void _colorconv_blit_32_to_8(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_8(src_rect, dest_rect, 4);
}



void _colorconv_blit_32_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_15(src_rect, dest_rect, 4);
}



void _colorconv_blit_32_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorconv_blit_true_to_16(src_rect, dest_rect, 4);
}



void _colorconv_blit_32_to_24(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   unsigned int temp;
   
   width = src_rect->width;
   src_feed = src_rect->pitch - (width << 2);
   dest_feed = dest_rect->pitch - (width * 3);
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width; x; x--) {
         temp = *(unsigned int *)src;
#ifdef ALLEGRO_LITTLE_ENDIAN
	 *(unsigned char *)dest = (unsigned char)(temp & 0xff);
	 *(unsigned char *)(dest + 1) = (unsigned char)(temp >> 8) & 0xff;
	 *(unsigned char *)(dest + 2) = (unsigned char)(temp >> 16) & 0xff;
#else
	 *(unsigned char *)(dest + 2) = (unsigned char)(temp & 0xff);
	 *(unsigned char *)(dest + 1) = (unsigned char)(temp >> 8) & 0xff;
	 *(unsigned char *)dest = (unsigned char)(temp >> 16) & 0xff;
#endif
	 src += 4;
	 dest += 3;
      }
      src += src_feed;
      dest += dest_feed;
   }
}


#endif

#ifndef ALLEGRO_NO_COLORCOPY


static void colorcopy(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
{
   unsigned char *src;
   unsigned char *dest;
   int width;
   int src_feed;
   int dest_feed;
   int y, x;
   
   width = src_rect->width * bpp;
   src_feed = src_rect->pitch - width;
   dest_feed = dest_rect->pitch - width;
   src = src_rect->data;
   dest = dest_rect->data;
   for (y = src_rect->height; y; y--) {
      for (x = width >> 2; x; x--) {
         *(unsigned int *)dest = *(unsigned int *)src;
	 src += 4;
	 dest += 4;
      }
      if (width & 0x2) {
         *(unsigned short *)dest = *(unsigned short *)src;
	 src += 2;
	 dest += 2;
      }
      if (width & 0x1) {
         *(unsigned char *)dest = *(unsigned char *)src;
	 src++;
	 dest++;
      }
      src += src_feed;
      dest += dest_feed;
   }
   
}



void _colorcopy_blit_15_to_15(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorcopy(src_rect, dest_rect, 2);
}



void _colorcopy_blit_16_to_16(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorcopy(src_rect, dest_rect, 2);
}



void _colorcopy_blit_24_to_24(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorcopy(src_rect, dest_rect, 3);
}



void _colorcopy_blit_32_to_32(struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
{
   colorcopy(src_rect, dest_rect, 4);
}


#endif

#endif
