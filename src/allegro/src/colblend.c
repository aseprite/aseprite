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
 *      Interpolation routines for hicolor and truecolor pixels.
 *
 *      By Cloud Wu and Burton Radons.
 *
 *      Alpha blending optimised by Peter Cech.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#define BLEND(bpp, r, g, b)   _blender_trans##bpp(makecol##bpp(r, g, b), y, n)

#define T(x, y, n)            (((y) - (x)) * (n) / 255 + (x))



/* _blender_black:
 *  Fallback routine for when we don't have anything better to do.
 */
unsigned long _blender_black(unsigned long x, unsigned long y, unsigned long n)
{
   return 0;
}



#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)



#if (defined ALLEGRO_NO_ASM) || (!defined ALLEGRO_I386) 
				    /* i386 asm version is in imisc.s */


/* _blender_trans24:
 *  24 bit trans blender function.
 */
unsigned long _blender_trans24(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long res, g;

   if (n)
      n++;

   res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
   y &= 0xFF00;
   x &= 0xFF00;
   g = (x - y) * n / 256 + y;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}


#endif      /* C version */



/* _blender_alpha24:
 *  Combines a 32 bit RGBA sprite with a 24 bit RGB destination.
 */
unsigned long _blender_alpha24(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long xx = makecol24(getr32(x), getg32(x), getb32(x));
   unsigned long res, g;

   n = geta32(x);

   if (n)
      n++;

   res = ((xx & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
   y &= 0xFF00;
   xx &= 0xFF00;
   g = (xx - y) * n / 256 + y;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}



/* _blender_alpha32:
 *  Combines a 32 bit RGBA sprite with a 32 bit RGB destination.
 */
unsigned long _blender_alpha32(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long res, g;

   n = geta32(x);

   if (n)
      n++;

   res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
   y &= 0xFF00;
   x &= 0xFF00;
   g = (x - y) * n / 256 + y;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}



/* _blender_alpha24_bgr:
 *  Combines a 32 bit RGBA sprite with a 24 bit RGB destination, optimised
 *  for when one is in a BGR format and the other is RGB.
 */
unsigned long _blender_alpha24_bgr(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long res, g;

   n = x >> 24;

   if (n)
      n++;

   x = ((x>>16)&0xFF) | (x&0xFF00) | ((x<<16)&0xFF0000);

   res = ((x & 0xFF00FF) - (y & 0xFF00FF)) * n / 256 + y;
   y &= 0xFF00;
   x &= 0xFF00;
   g = (x - y) * n / 256 + y;

   res &= 0xFF00FF;
   g &= 0xFF00;

   return res | g;
}



/* _blender_add24:
 *  24 bit additive blender function.
 */
unsigned long _blender_add24(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr24(y) + getr24(x) * n / 256;
   int g = getg24(y) + getg24(x) * n / 256;
   int b = getb24(y) + getb24(x) * n / 256;

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol24(r, g, b);
}



/* _blender_burn24:
 *  24 bit burn blender function.
 */
unsigned long _blender_burn24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, MAX(getr24(x) - getr24(y), 0),
		    MAX(getg24(x) - getg24(y), 0),
		    MAX(getb24(x) - getb24(y), 0));
}



/* _blender_color24:
 *  24 bit color blender function.
 */
unsigned long _blender_color24(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr24(x), getg24(x), getb24(x), &xh, &xs, &xv);
   rgb_to_hsv(getr24(y), getg24(y), getb24(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);
   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol24(r, g, b);
}



/* _blender_difference24:
 *  24 bit difference blender function.
 */
unsigned long _blender_difference24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, ABS(getr24(y) - getr24(x)),
		    ABS(getg24(y) - getg24(x)),
		    ABS(getb24(y) - getb24(x)));
}



/* _blender_dissolve24:
 *  24 bit dissolve blender function.
 */
unsigned long _blender_dissolve24(unsigned long x, unsigned long y, unsigned long n)
{
   if (n == 255)
      return x;

   return ((_al_rand() & 255) < (int)n) ? x : y;
}



/* _blender_dodge24:
 *  24 bit dodge blender function.
 */
unsigned long _blender_dodge24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, getr24(x) + (getr24(y) * n / 256),
		    getg24(x) + (getg24(y) * n / 256),
		    getb24(x) + (getb24(y) * n / 256));
}



/* _blender_hue24:
 *  24 bit hue blender function.
 */
unsigned long _blender_hue24(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr24(x), getg24(x), getb24(x), &xh, &xs, &xv);
   rgb_to_hsv(getr24(y), getg24(y), getb24(y), &yh, &ys, &yv);

   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol24(r, g, b);
}



/* _blender_invert24:
 *  24 bit invert blender function.
 */
unsigned long _blender_invert24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, 255-getr24(x), 255-getg24(x), 255-getb24(x));
}



/* _blender_luminance24:
 *  24 bit luminance blender function.
 */
unsigned long _blender_luminance24(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr24(x), getg24(x), getb24(x), &xh, &xs, &xv);
   rgb_to_hsv(getr24(y), getg24(y), getb24(y), &yh, &ys, &yv);

   xv = T(xv, yv, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol24(r, g, b);
}



/* _blender_multiply24:
 *  24 bit multiply blender function.
 */
unsigned long _blender_multiply24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, getr24(x) * getr24(y) / 256, 
		    getg24(x) * getg24(y) / 256, 
		    getb24(x) * getb24(y) / 256);
}



/* _blender_saturation24:
 *  24 bit saturation blender function.
 */
unsigned long _blender_saturation24(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr24(x), getg24(x), getb24(x), &xh, &xs, &xv);
   rgb_to_hsv(getr24(y), getg24(y), getb24(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol24(r, g, b);
}



/* _blender_screen24:
 *  24 bit screen blender function.
 */
unsigned long _blender_screen24(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(24, 255 - ((255 - getr24(x)) * (255 - getr24(y))) / 256,
		    255 - ((255 - getg24(x)) * (255 - getg24(y))) / 256,
		    255 - ((255 - getb24(x)) * (255 - getb24(y))) / 256);
}



#endif      /* end of 24/32 bit routines */


#if (defined ALLEGRO_COLOR15) || (defined ALLEGRO_COLOR16)



/* _blender_trans16:
 *  16 bit trans blender function.
 */
unsigned long _blender_trans16(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   if (n)
      n = (n + 1) / 8;

   x = ((x & 0xFFFF) | (x << 16)) & 0x7E0F81F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x7E0F81F;

   result = ((x - y) * n / 32 + y) & 0x7E0F81F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha16:
 *  Combines a 32 bit RGBA sprite with a 16 bit RGB destination.
 */
unsigned long _blender_alpha16(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = geta32(x);

   if (n)
      n = (n + 1) / 8;

   x = makecol16(getr32(x), getg32(x), getb32(x));

   x = (x | (x << 16)) & 0x7E0F81F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x7E0F81F;

   result = ((x - y) * n / 32 + y) & 0x7E0F81F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha16_rgb
 *  Combines a 32 bit RGBA sprite with a 16 bit RGB destination, optimised
 *  for when both pixels are in an RGB layout.
 */
unsigned long _blender_alpha16_rgb(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = x >> 24;

   if (n)
      n = (n + 1) / 8;

   x = ((x>>3)&0x001F) | ((x>>5)&0x07E0) | ((x>>8)&0xF800);

   x = (x | (x << 16)) & 0x7E0F81F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x7E0F81F;

   result = ((x - y) * n / 32 + y) & 0x7E0F81F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha16_bgr
 *  Combines a 32 bit RGBA sprite with a 16 bit RGB destination, optimised
 *  for when one pixel is in an RGB layout and the other is BGR.
 */
unsigned long _blender_alpha16_bgr(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = x >> 24;

   if (n)
      n = (n + 1) / 8;

   x = ((x>>19)&0x001F) | ((x>>5)&0x07E0) | ((x<<8)&0xF800);

   x = (x | (x << 16)) & 0x7E0F81F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x7E0F81F;

   result = ((x - y) * n / 32 + y) & 0x7E0F81F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_add16:
 *  16 bit additive blender function.
 */
unsigned long _blender_add16(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr16(y) + getr16(x) * n / 256;
   int g = getg16(y) + getg16(x) * n / 256;
   int b = getb16(y) + getb16(x) * n / 256;

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol16(r, g, b);
}



/* _blender_burn16:
 *  16 bit burn blender function.
 */
unsigned long _blender_burn16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, MAX(getr16(x) - getr16(y), 0),
		    MAX(getg16(x) - getg16(y), 0),
		    MAX(getb16(x) - getb16(y), 0));
}



/* _blender_color16:
 *  16 bit color blender function.
 */
unsigned long _blender_color16(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
   rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);
   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol16(r, g, b);
}



/* _blender_difference16:
 *  16 bit difference blender function.
 */
unsigned long _blender_difference16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, ABS(getr16(y) - getr16(x)),
		    ABS(getg16(y) - getg16(x)),
		    ABS(getb16(y) - getb16(x)));
}



/* _blender_dissolve16:
 *  16 bit dissolve blender function.
 */
unsigned long _blender_dissolve16(unsigned long x, unsigned long y, unsigned long n)
{
   if (n == 255)
      return x;

   return ((_al_rand() & 255) < (int)n) ? x : y;
}



/* _blender_dodge16:
 *  16 bit dodge blender function.
 */
unsigned long _blender_dodge16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, getr16(x) + (getr16(y) * n / 256),
		    getg16(x) + (getg16(y) * n / 256),
		    getb16(x) + (getb16(y) * n / 256));
}



/* _blender_hue16:
 *  16 bit hue blender function.
 */
unsigned long _blender_hue16(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
   rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol16(r, g, b);
}



/* _blender_invert16:
 *  16 bit invert blender function.
 */
unsigned long _blender_invert16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, 255-getr16(x), 255-getg16(x), 255-getb16(x));
}



/* _blender_luminance16:
 *  16 bit luminance blender function.
 */
unsigned long _blender_luminance16(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
   rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

   xv = T(xv, yv, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol16(r, g, b);
}



/* _blender_multiply16:
 *  16 bit multiply blender function.
 */
unsigned long _blender_multiply16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, getr16(x) * getr16(y) / 256, 
		    getg16(x) * getg16(y) / 256, 
		    getb16(x) * getb16(y) / 256);
}



/* _blender_saturation16:
 *  16 bit saturation blender function.
 */
unsigned long _blender_saturation16(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr16(x), getg16(x), getb16(x), &xh, &xs, &xv);
   rgb_to_hsv(getr16(y), getg16(y), getb16(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol16(r, g, b);
}



/* _blender_screen16:
 *  16 bit screen blender function.
 */
unsigned long _blender_screen16(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(16, 255 - ((255 - getr16(x)) * (255 - getr16(y))) / 256,
		    255 - ((255 - getg16(x)) * (255 - getg16(y))) / 256,
		    255 - ((255 - getb16(x)) * (255 - getb16(y))) / 256);
}



/* _blender_trans15:
 *  15 bit trans blender function.
 */
unsigned long _blender_trans15(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   if (n)
      n = (n + 1) / 8;

   x = ((x & 0xFFFF) | (x << 16)) & 0x3E07C1F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x3E07C1F;

   result = ((x - y) * n / 32 + y) & 0x3E07C1F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha15:
 *  Combines a 32 bit RGBA sprite with a 15 bit RGB destination.
 */
unsigned long _blender_alpha15(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = geta32(x);

   if (n)
      n = (n + 1) / 8;

   x = makecol15(getr32(x), getg32(x), getb32(x));

   x = (x | (x << 16)) & 0x3E07C1F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x3E07C1F;

   result = ((x - y) * n / 32 + y) & 0x3E07C1F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha15_rgb
 *  Combines a 32 bit RGBA sprite with a 15 bit RGB destination, optimised
 *  for when both pixels are in an RGB layout.
 */
unsigned long _blender_alpha15_rgb(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = x >> 24;

   if (n)
      n = (n + 1) / 8;

   x = ((x>>3)&0x001F) | ((x>>6)&0x03E0) | ((x>>9)&0xEC00);

   x = (x | (x << 16)) & 0x3E07C1F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x3E07C1F;

   result = ((x - y) * n / 32 + y) & 0x3E07C1F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_alpha15_bgr
 *  Combines a 32 bit RGBA sprite with a 15 bit RGB destination, optimised
 *  for when one pixel is in an RGB layout and the other is BGR.
 */
unsigned long _blender_alpha15_bgr(unsigned long x, unsigned long y, unsigned long n)
{
   unsigned long result;

   n = x >> 24;

   if (n)
      n = (n + 1) / 8;

   x = ((x>>19)&0x001F) | ((x>>6)&0x03E0) | ((x<<7)&0xEC00);

   x = (x | (x << 16)) & 0x3E07C1F;
   y = ((y & 0xFFFF) | (y << 16)) & 0x3E07C1F;

   result = ((x - y) * n / 32 + y) & 0x3E07C1F;

   return ((result & 0xFFFF) | (result >> 16));
}



/* _blender_add15:
 *  15 bit additive blender function.
 */
unsigned long _blender_add15(unsigned long x, unsigned long y, unsigned long n)
{
   int r = getr15(y) + getr15(x) * n / 256;
   int g = getg15(y) + getg15(x) * n / 256;
   int b = getb15(y) + getb15(x) * n / 256;

   r = MIN(r, 255);
   g = MIN(g, 255);
   b = MIN(b, 255);

   return makecol15(r, g, b);
}



/* _blender_burn15:
 *  15 bit burn blender function.
 */
unsigned long _blender_burn15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, MAX(getr15(x) - getr15(y), 0),
		    MAX(getg15(x) - getg15(y), 0),
		    MAX(getb15(x) - getb15(y), 0));
}



/* _blender_color15:
 *  15 bit color blender function.
 */
unsigned long _blender_color15(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
   rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);
   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol15(r, g, b);
}



/* _blender_difference15:
 *  15 bit difference blender function.
 */
unsigned long _blender_difference15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, ABS(getr15(y) - getr15(x)),
		    ABS(getg15(y) - getg15(x)),
		    ABS(getb15(y) - getb15(x)));
}



/* _blender_dissolve15:
 *  15 bit dissolve blender function.
 */
unsigned long _blender_dissolve15(unsigned long x, unsigned long y, unsigned long n)
{
   if (n == 255)
      return x;

   return ((_al_rand() & 255) < (int)n) ? x : y;
}



/* _blender_dodge15:
 *  15 bit dodge blender function.
 */
unsigned long _blender_dodge15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, getr15(x) + (getr15(y) * n / 256),
		    getg15(x) + (getg15(y) * n / 256),
		    getb15(x) + (getb15(y) * n / 256));
}



/* _blender_hue15:
 *  15 bit hue blender function.
 */
unsigned long _blender_hue15(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
   rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

   xh = T(xh, yh, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol15(r, g, b);
}



/* _blender_invert15:
 *  15 bit invert blender function.
 */
unsigned long _blender_invert15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, 255-getr15(x), 255-getg15(x), 255-getb15(x));
}



/* _blender_luminance15:
 *  15 bit luminance blender function.
 */
unsigned long _blender_luminance15(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
   rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

   xv = T(xv, yv, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol15(r, g, b);
}



/* _blender_multiply15:
 *  15 bit multiply blender function.
 */
unsigned long _blender_multiply15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, getr15(x) * getr15(y) / 256, 
		    getg15(x) * getg15(y) / 256, 
		    getb15(x) * getb15(y) / 256);
}



/* _blender_saturation15:
 *  15 bit saturation blender function.
 */
unsigned long _blender_saturation15(unsigned long x, unsigned long y, unsigned long n)
{
   float xh, xs, xv;
   float yh, ys, yv;
   int r, g, b;

   rgb_to_hsv(getr15(x), getg15(x), getb15(x), &xh, &xs, &xv);
   rgb_to_hsv(getr15(y), getg15(y), getb15(y), &yh, &ys, &yv);

   xs = T(xs, ys, n);

   hsv_to_rgb(xh, xs, xv, &r, &g, &b);

   return makecol15(r, g, b);
}



/* _blender_screen15:
 *  15 bit screen blender function.
 */
unsigned long _blender_screen15(unsigned long x, unsigned long y, unsigned long n)
{
   return BLEND(15, 255 - ((255 - getr15(x)) * (255 - getr15(y))) / 256,
		    255 - ((255 - getg15(x)) * (255 - getg15(y))) / 256,
		    255 - ((255 - getb15(x)) * (255 - getb15(y))) / 256);
}



#endif      /* end of 15/16 bit routines */



#ifdef ALLEGRO_COLOR16
   #define BF16(name)   name
#else
   #define BF16(name)   _blender_black
#endif


#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)
   #define BF24(name)   name
#else
   #define BF24(name)   _blender_black
#endif



/* these functions are all the same, so we can generate them with a macro */
#define SET_BLENDER_FUNC(name)                                 \
   void set_##name##_blender(int r, int g, int b, int a)       \
   {                                                           \
      if (gfx_driver && gfx_driver->set_blender_mode)          \
         gfx_driver->set_blender_mode(blender_mode_##name, r, g, b, a);\
      set_blender_mode(BF16(_blender_##name##15),              \
		       BF16(_blender_##name##16),              \
		       BF24(_blender_##name##24),              \
		       r, g, b, a);                            \
   }


SET_BLENDER_FUNC(trans);
SET_BLENDER_FUNC(add);
SET_BLENDER_FUNC(burn);
SET_BLENDER_FUNC(color);
SET_BLENDER_FUNC(difference);
SET_BLENDER_FUNC(dissolve);
SET_BLENDER_FUNC(dodge);
SET_BLENDER_FUNC(hue);
SET_BLENDER_FUNC(invert);
SET_BLENDER_FUNC(luminance);
SET_BLENDER_FUNC(multiply);
SET_BLENDER_FUNC(saturation);
SET_BLENDER_FUNC(screen);



/* set_alpha_blender:
 *  Sets the special RGBA blending mode.
 */
void set_alpha_blender(void)
{
   BLENDER_FUNC f15, f16, f24, f32;
   int r, b;

   /* Call gfx_driver->set_blender_mode() for hardware acceleration */
   if (gfx_driver && gfx_driver->set_blender_mode)
      gfx_driver->set_blender_mode(blender_mode_alpha, 0, 0, 0, 0);

   /* check which way around the 32 bit pixels are */
   if ((_rgb_g_shift_32 == 8) && (_rgb_a_shift_32 == 24)) {
      r = (_rgb_r_shift_32) ? 1 : 0;
      b = (_rgb_b_shift_32) ? 1 : 0;
   }
   else
      r = b = 0;

   #ifdef ALLEGRO_COLOR16

      /* decide which 15 bit blender to use */
      if ((_rgb_r_shift_15 == r*10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == b*10))
	 f15 = _blender_alpha15_rgb;
      else if ((_rgb_r_shift_15 == b*10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == r*10))
	 f15 = _blender_alpha15_bgr;
      else
	 f15 = _blender_alpha15;

      /* decide which 16 bit blender to use */
      if ((_rgb_r_shift_16 == r*11) && (_rgb_g_shift_16 == 5) && (_rgb_b_shift_16 == b*11))
	 f16 = _blender_alpha16_rgb;
      else if ((_rgb_r_shift_16 == b*11) && (_rgb_g_shift_16 == 5) && (_rgb_b_shift_16 == r*11))
	 f16 = _blender_alpha16_bgr;
      else
	 f16 = _blender_alpha16;

   #else

      /* hicolor not included in this build */
      f15 = _blender_black;
      f16 = _blender_black;

   #endif

   #ifdef ALLEGRO_COLOR24

      /* decide which 24 bit blender to use */
      if ((_rgb_r_shift_24 == r*16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == b*16))
	 f24 = _blender_alpha32;
      else if ((_rgb_r_shift_24 == b*16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == r*16))
	 f24 = _blender_alpha24_bgr;
      else
	 f24 = _blender_alpha24;

   #else

      /* 24 bit color not included in this build */
      f24 = _blender_black;

   #endif

   #ifdef ALLEGRO_COLOR32
      f32 = _blender_alpha32;
   #else
      f32 = _blender_black;
   #endif

   set_blender_mode_ex(_blender_black, _blender_black, _blender_black,
		       f32, f15, f16, f24, 0, 0, 0, 0);
}



#ifdef ALLEGRO_COLOR32

/* _blender_write_alpha:
 *  Overlays an alpha channel onto an existing 32 bit RGBA bitmap.
 */
unsigned long _blender_write_alpha(unsigned long x, unsigned long y, unsigned long n)
{
   return (y & 0xFFFFFF) | (x << 24);
}

#endif



/* set_write_alpha_blender:
 *  Sets the special RGBA editing mode.
 */
void set_write_alpha_blender(void)
{
   BLENDER_FUNC f32;

   #ifdef ALLEGRO_COLOR32
      f32 = _blender_write_alpha;
   #else
      f32 = _blender_black;
   #endif

   set_blender_mode_ex(_blender_black, _blender_black, _blender_black,
		       f32, 
		       _blender_black, _blender_black, _blender_black,
		       0, 0, 0, 0);
}

