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
 *      Color manipulation routines (blending, format conversion, lighting
 *      table construction, etc).
 *
 *      By Shawn Hargreaves.
 *
 *      Dave Thomson contributed the RGB <-> HSV conversion routines.
 *
 *      Jan Hubicka wrote the super-fast version of create_rgb_table().
 *
 *      Michael Bevin and Sven Sandberg optimised the create_trans_table()
 *      function.
 *
 *      Sven Sandberg optimised the create_light_table() function.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>
#include <string.h>
#include <math.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* makecol_depth:
 *  Converts R, G, and B values (ranging 0-255) to whatever pixel format
 *  is required by the specified color depth.
 */
int makecol_depth(int color_depth, int r, int g, int b)
{
   switch (color_depth) {

      case 8:
         return makecol8(r, g, b);

      case 15:
         return makecol15(r, g, b);

      case 16:
         return makecol16(r, g, b);

      case 24:
         return makecol24(r, g, b);

      case 32:
         return makecol32(r, g, b);
   }

   return 0;
}



/* makeacol_depth:
 *  Converts R, G, B, and A values (ranging 0-255) to whatever pixel format
 *  is required by the specified color depth.
 */
int makeacol_depth(int color_depth, int r, int g, int b, int a)
{
   switch (color_depth) {

      case 8:
         return makecol8(r, g, b);

      case 15:
         return makecol15(r, g, b);

      case 16:
         return makecol16(r, g, b);

      case 24:
         return makecol24(r, g, b);

      case 32:
         return makeacol32(r, g, b, a);
   }

   return 0;
}



/* makecol:
 *  Converts R, G, and B values (ranging 0-255) to whatever pixel format
 *  is required by the current video mode.
 */
int makecol(int r, int g, int b)
{
   return makecol_depth(_color_depth, r, g, b);
}



/* makeacol:
 *  Converts R, G, B, and A values (ranging 0-255) to whatever pixel format
 *  is required by the current video mode.
 */
int makeacol(int r, int g, int b, int a)
{
   return makeacol_depth(_color_depth, r, g, b, a);
}



/* getr_depth:
 *  Extracts the red component (ranging 0-255) from a pixel in the format
 *  being used by the specified color depth.
 */
int getr_depth(int color_depth, int c)
{
   switch (color_depth) {

      case 8:
         return getr8(c);

      case 15:
         return getr15(c);

      case 16:
         return getr16(c);

      case 24:
         return getr24(c);

      case 32:
         return getr32(c);
   }

   return 0;
}



/* getg_depth:
 *  Extracts the green component (ranging 0-255) from a pixel in the format
 *  being used by the specified color depth.
 */
int getg_depth(int color_depth, int c)
{
   switch (color_depth) {

      case 8:
         return getg8(c);

      case 15:
         return getg15(c);

      case 16:
         return getg16(c);

      case 24:
         return getg24(c);

      case 32:
         return getg32(c);
   }

   return 0;
}



/* getb_depth:
 *  Extracts the blue component (ranging 0-255) from a pixel in the format
 *  being used by the specified color depth.
 */
int getb_depth(int color_depth, int c)
{
   switch (color_depth) {

      case 8:
         return getb8(c);

      case 15:
         return getb15(c);

      case 16:
         return getb16(c);

      case 24:
         return getb24(c);

      case 32:
         return getb32(c);
   }

   return 0;
}



/* geta_depth:
 *  Extracts the alpha component (ranging 0-255) from a pixel in the format
 *  being used by the specified color depth.
 */
int geta_depth(int color_depth, int c)
{
   if (color_depth == 32)
      return geta32(c);

   /* dacap: This should return 255, anyway as the Debian port uses
      the official Allegro library, we cannot depend on this function. */
   return 0;
}



/* getr:
 *  Extracts the red component (ranging 0-255) from a pixel in the format
 *  being used by the current video mode.
 */
int getr(int c)
{
   return getr_depth(_color_depth, c);
}



/* getg:
 *  Extracts the green component (ranging 0-255) from a pixel in the format
 *  being used by the current video mode.
 */
int getg(int c)
{
   return getg_depth(_color_depth, c);
}



/* getb:
 *  Extracts the blue component (ranging 0-255) from a pixel in the format
 *  being used by the current video mode.
 */
int getb(int c)
{
   return getb_depth(_color_depth, c);
}



/* geta:
 *  Extracts the alpha component (ranging 0-255) from a pixel in the format
 *  being used by the current video mode.
 */
int geta(int c)
{
   return geta_depth(_color_depth, c);
}



/* 1.5k lookup table for color matching */
static unsigned int col_diff[3*128];



/* bestfit_init:
 *  Color matching is done with weighted squares, which are much faster
 *  if we pregenerate a little lookup table...
 */
static void bestfit_init(void)
{
   int i;

   for (i=1; i<64; i++) {
      int k = i * i;
      col_diff[0  +i] = col_diff[0  +128-i] = k * (59 * 59);
      col_diff[128+i] = col_diff[128+128-i] = k * (30 * 30);
      col_diff[256+i] = col_diff[256+128-i] = k * (11 * 11);
   }
}



/* bestfit_color:
 *  Searches a palette for the color closest to the requested R, G, B value.
 */
int bestfit_color(AL_CONST PALETTE pal, int r, int g, int b)
{
   int i, coldiff, lowest, bestfit;

   ASSERT(r >= 0 && r <= 63);
   ASSERT(g >= 0 && g <= 63);
   ASSERT(b >= 0 && b <= 63);

   if (col_diff[1] == 0)
      bestfit_init();

   bestfit = 0;
   lowest = INT_MAX;

   /* only the transparent (pink) color can be mapped to index 0 */
   if ((r == 63) && (g == 0) && (b == 63))
      i = 0;
   else
      i = 1;

   while (i<PAL_SIZE) {
      AL_CONST RGB *rgb = &pal[i];
      coldiff = (col_diff + 0) [ (rgb->g - g) & 0x7F ];
      if (coldiff < lowest) {
         coldiff += (col_diff + 128) [ (rgb->r - r) & 0x7F ];
         if (coldiff < lowest) {
            coldiff += (col_diff + 256) [ (rgb->b - b) & 0x7F ];
            if (coldiff < lowest) {
               bestfit = rgb - pal;    /* faster than `bestfit = i;' */
               if (coldiff == 0)
                  return bestfit;
               lowest = coldiff;
            }
         }
      }
      i++;
   }

   return bestfit;
}



/* makecol8:
 *  Converts R, G, and B values (ranging 0-255) to an 8 bit paletted color.
 *  If the global rgb_map table is initialised, it uses that, otherwise
 *  it searches through the current palette to find the best match.
 */
int makecol8(int r, int g, int b)
{
   if (rgb_map)
      return rgb_map->data[r>>3][g>>3][b>>3];
   else
      return bestfit_color(_current_palette, r>>2, g>>2, b>>2);
}



/* hsv_to_rgb:
 *  Converts from HSV colorspace to RGB values.
 */
void hsv_to_rgb(float h, float s, float v, int *r, int *g, int *b)
{
   float f, x, y, z;
   int i;

   ASSERT(s >= 0 && s <= 1);
   ASSERT(v >= 0 && v <= 1);

   v *= 255.0f;

   if (s == 0.0f) { /* ok since we don't divide by s, and faster */
      *r = *g = *b = v + 0.5f;
   }
   else {
      h = fmod(h, 360.0f) / 60.0f;
      if (h < 0.0f)
         h += 6.0f;

      i = (int)h;
      f = h - i;
      x = v * s;
      y = x * f;
      v += 0.5f; /* round to the nearest integer below */
      z = v - x;

      switch (i) {

         case 6:
         case 0:
            *r = v;
            *g = z + y;
            *b = z;
            break;

         case 1:
            *r = v - y;
            *g = v;
            *b = z;
            break;

         case 2:
            *r = z;
            *g = v;
            *b = z + y;
            break;

         case 3:
            *r = z;
            *g = v - y;
            *b = v;
            break;

         case 4:
            *r = z + y;
            *g = z;
            *b = v;
            break;

         case 5:
            *r = v;
            *g = z;
            *b = v - y;
            break;
      }
   }
}



/* rgb_to_hsv:
 *  Converts an RGB value into the HSV colorspace.
 */
void rgb_to_hsv(int r, int g, int b, float *h, float *s, float *v)
{
   int delta;

   ASSERT(r >= 0 && r <= 255);
   ASSERT(g >= 0 && g <= 255);
   ASSERT(b >= 0 && b <= 255);

   if (r > g) {
      if (b > r) {
         /* b>r>g */
         delta = b-g;
         *h = 240.0f + ((r-g) * 60) / (float)delta;
         *s = (float)delta / (float)b;
         *v = (float)b * (1.0f/255.0f);
      }
      else {
         /* r>g and r>b */
         delta = r - MIN(g, b);
         *h = ((g-b) * 60) / (float)delta;
         if (*h < 0.0f)
            *h += 360.0f;
         *s = (float)delta / (float)r;
         *v = (float)r * (1.0f/255.0f);
      }
   }
   else {
      if (b > g) {
         /* b>g>=r */
         delta = b-r;
         *h = 240.0f + ((r-g) * 60) / (float)delta;
         *s = (float)delta / (float)b;
         *v = (float)b * (1.0f/255.0f);
      }
      else {
         /* g>=b and g>=r */
         delta = g - MIN(r, b);
         if (delta == 0) {
            *h = 0.0f;
            if (g == 0)
               *s = *v = 0.0f;
            else {
               *s = (float)delta / (float)g;
               *v = (float)g * (1.0f/255.0f);
            }
         }
         else {
            *h = 120.0f + ((b-r) * 60) / (float)delta;
            *s = (float)delta / (float)g;
            *v = (float)g * (1.0f/255.0f);
         }
      }
   }
}



/* create_rgb_table:
 *  Fills an RGB_MAP lookup table with conversion data for the specified
 *  palette. This is the faster version by Jan Hubicka.
 *
 *  Uses alg. similar to floodfill - it adds one seed per every color in
 *  palette to its best position. Then areas around seed are filled by
 *  same color because it is best approximation for them, and then areas
 *  about them etc...
 *
 *  It does just about 80000 tests for distances and this is about 100
 *  times better than normal 256*32000 tests so the calculation time
 *  is now less than one second at all computers I tested.
 */
void create_rgb_table(RGB_MAP *table, AL_CONST PALETTE pal, void (*callback)(int pos))
{
   #define UNUSED 65535
   #define LAST 65532

   /* macro add adds to single linked list */
   #define add(i)    (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                     (first != LAST ? (next[last] = (i)) : (first = (i))), \
                     (last = (i))) : 0)

   /* same but w/o checking for first element */
   #define add1(i)   (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                     next[last] = (i), \
                     (last = (i))) : 0)

   /* calculates distance between two colors */
   #define dist(a1, a2, a3, b1, b2, b3) \
                     (col_diff[ ((a2) - (b2)) & 0x7F] + \
                     (col_diff + 128)[((a1) - (b1)) & 0x7F] + \
                     (col_diff + 256)[((a3) - (b3)) & 0x7F])

   /* converts r,g,b to position in array and back */
   #define pos(r, g, b) \
                     (((r) / 2) * 32 * 32 + ((g) / 2) * 32 + ((b) / 2))

   #define depos(pal, r, g, b) \
                     ((b) = ((pal) & 31) * 2, \
                      (g) = (((pal) >> 5) & 31) * 2, \
                      (r) = (((pal) >> 10) & 31) * 2)

   /* is current color better than pal1? */
   #define better(r1, g1, b1, pal1) \
                     (((int)dist((r1), (g1), (b1), \
                                 (pal1).r, (pal1).g, (pal1).b)) > (int)dist2)

   /* checking of position */
   #define dopos(rp, gp, bp, ts) \
      if ((rp > -1 || r > 0) && (rp < 1 || r < 61) && \
          (gp > -1 || g > 0) && (gp < 1 || g < 61) && \
          (bp > -1 || b > 0) && (bp < 1 || b < 61)) { \
         i = first + rp * 32 * 32 + gp * 32 + bp; \
         if (!data[i]) { \
            data[i] = val; \
            add1(i); \
         } \
         else if ((ts) && (data[i] != val)) { \
            dist2 = (rp ? (col_diff+128)[(r+2*rp-pal[val].r) & 0x7F] : r2) + \
                    (gp ? (col_diff    )[(g+2*gp-pal[val].g) & 0x7F] : g2) + \
                    (bp ? (col_diff+256)[(b+2*bp-pal[val].b) & 0x7F] : b2); \
            if (better((r+2*rp), (g+2*gp), (b+2*bp), pal[data[i]])) { \
               data[i] = val; \
               add1(i); \
            } \
         } \
      }

   int i, curr, r, g, b, val, dist2;
   unsigned int r2, g2, b2;
   unsigned short next[32*32*32];
   unsigned char *data;
   int first = LAST;
   int last = LAST;
   int count = 0;
   int cbcount = 0;

   #define AVERAGE_COUNT   18000

   if (col_diff[1] == 0)
      bestfit_init();

   memset(next, 255, sizeof(next));
   memset(table->data, 0, sizeof(char)*32*32*32);

   data = (unsigned char *)table->data;

   /* add starting seeds for floodfill */
   for (i=1; i<PAL_SIZE; i++) {
      curr = pos(pal[i].r, pal[i].g, pal[i].b);
      if (next[curr] == UNUSED) {
         data[curr] = i;
         add(curr);
      }
   }

   /* main floodfill: two versions of loop for faster growing in blue axis */
   while (first != LAST) {
      depos(first, r, g, b);

      /* calculate distance of current color */
      val = data[first];
      r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
      g2 = (col_diff    )[((pal[val].g)-(g)) & 0x7F];
      b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];

      /* try to grow to all directions */
      dopos( 0, 0, 1, 1);
      dopos( 0, 0,-1, 1);
      dopos( 1, 0, 0, 1);
      dopos(-1, 0, 0, 1);
      dopos( 0, 1, 0, 1);
      dopos( 0,-1, 0, 1);

      /* faster growing of blue direction */
      if ((b > 0) && (data[first-1] == val)) {
         b -= 2;
         first--;
         b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];

         dopos(-1, 0, 0, 0);
         dopos( 1, 0, 0, 0);
         dopos( 0,-1, 0, 0);
         dopos( 0, 1, 0, 0);

         first++;
      }

      /* get next from list */
      i = first;
      first = next[first];
      next[i] = UNUSED;

      /* second version of loop */
      if (first != LAST) {
         depos(first, r, g, b);

         val = data[first];
         r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
         g2 = (col_diff    )[((pal[val].g)-(g)) & 0x7F];
         b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];

         dopos( 0, 0, 1, 1);
         dopos( 0, 0,-1, 1);
         dopos( 1, 0, 0, 1);
         dopos(-1, 0, 0, 1);
         dopos( 0, 1, 0, 1);
         dopos( 0,-1, 0, 1);

         if ((b < 61) && (data[first + 1] == val)) {
            b += 2;
            first++;
            b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7f];

            dopos(-1, 0, 0, 0);
            dopos( 1, 0, 0, 0);
            dopos( 0,-1, 0, 0);
            dopos( 0, 1, 0, 0);

            first--;
         }

         i = first;
         first = next[first];
         next[i] = UNUSED;
      }

      count++;
      if (count == (cbcount+1)*AVERAGE_COUNT/256) {
         if (cbcount < 256) {
            if (callback)
               callback(cbcount);
            cbcount++;
         }
      }
   }

   /* only the transparent (pink) color can be mapped to index 0 */
   if ((pal[0].r == 63) && (pal[0].g == 0) && (pal[0].b == 63))
      table->data[31][0][31] = 0;

   if (callback)
      while (cbcount < 256)
         callback(cbcount++);
}



/* create_light_table:
 *  Constructs a lighting color table for the specified palette. At light
 *  intensity 255 the table will produce the palette colors directly, and
 *  at level 0 it will produce the specified R, G, B value for all colors
 *  (this is specified in 0-63 VGA format). If the callback function is
 *  not NULL, it will be called 256 times during the calculation, allowing
 *  you to display a progress indicator.
 */
void create_light_table(COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, void (*callback)(int pos))
{
   int r1, g1, b1, r2, g2, b2, x, y;
   unsigned int t1, t2;

   ASSERT(table);
   ASSERT(r >= 0 && r <= 63);
   ASSERT(g >= 0 && g <= 63);
   ASSERT(b >= 0 && b <= 63);

   if (rgb_map) {
      for (x=0; x<PAL_SIZE-1; x++) {
         t1 = x * 0x010101;
         t2 = 0xFFFFFF - t1;

         r1 = (1 << 24) + r * t2;
         g1 = (1 << 24) + g * t2;
         b1 = (1 << 24) + b * t2;

         for (y=0; y<PAL_SIZE; y++) {
            r2 = (r1 + pal[y].r * t1) >> 25;
            g2 = (g1 + pal[y].g * t1) >> 25;
            b2 = (b1 + pal[y].b * t1) >> 25;

            table->data[x][y] = rgb_map->data[r2][g2][b2];
         }
      }
      if (callback)
         (*callback)(x);
   }
   else {
      for (x=0; x<PAL_SIZE-1; x++) {
         t1 = x * 0x010101;
         t2 = 0xFFFFFF - t1;

         r1 = (1 << 23) + r * t2;
         g1 = (1 << 23) + g * t2;
         b1 = (1 << 23) + b * t2;

         for (y=0; y<PAL_SIZE; y++) {
            r2 = (r1 + pal[y].r * t1) >> 24;
            g2 = (g1 + pal[y].g * t1) >> 24;
            b2 = (b1 + pal[y].b * t1) >> 24;

            table->data[x][y] = bestfit_color(pal, r2, g2, b2);
         }
      }

      if (callback)
         (*callback)(x);
   }

   for (y=0; y<PAL_SIZE; y++)
      table->data[255][y] = y;
}



/* create_trans_table:
 *  Constructs a translucency color table for the specified palette. The
 *  r, g, and b parameters specifiy the solidity of each color component,
 *  ranging from 0 (totally transparent) to 255 (totally solid). Source
 *  color #0 is a special case, and is set to leave the destination
 *  unchanged, so that masked sprites will draw correctly. If the callback
 *  function is not NULL, it will be called 256 times during the calculation,
 *  allowing you to display a progress indicator.
 */
void create_trans_table(COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, void (*callback)(int pos))
{
   int tmp[768], *q;
   int x, y, i, j, k;
   unsigned char *p;
   int tr, tg, tb;
   int add;

   ASSERT(table);
   ASSERT(r >= 0 && r <= 255);
   ASSERT(g >= 0 && g <= 255);
   ASSERT(b >= 0 && b <= 255);

   /* This is a bit ugly, but accounts for the solidity parameters
      being in the range 0-255 rather than 0-256. Given that the
      precision of r,g,b components is only 6 bits it shouldn't do any
      harm. */
   if (r > 128)
      r++;
   if (g > 128)
      g++;
   if (b > 128)
      b++;

   if (rgb_map)
      add = 255;
   else
      add = 127;

   for (x=0; x<256; x++) {
      tmp[x*3]   = pal[x].r * (256-r) + add;
      tmp[x*3+1] = pal[x].g * (256-g) + add;
      tmp[x*3+2] = pal[x].b * (256-b) + add;
   }

   for (x=1; x<PAL_SIZE; x++) {
      i = pal[x].r * r;
      j = pal[x].g * g;
      k = pal[x].b * b;

      p = table->data[x];
      q = tmp;

      if (rgb_map) {
         for (y=0; y<PAL_SIZE; y++) {
            tr = (i + *(q++)) >> 9;
            tg = (j + *(q++)) >> 9;
            tb = (k + *(q++)) >> 9;
            p[y] = rgb_map->data[tr][tg][tb];
         }
      }
      else {
         for (y=0; y<PAL_SIZE; y++) {
            tr = (i + *(q++)) >> 8;
            tg = (j + *(q++)) >> 8;
            tb = (k + *(q++)) >> 8;
            p[y] = bestfit_color(pal, tr, tg, tb);
         }
      }

      if (callback)
         (*callback)(x-1);
   }

   for (y=0; y<PAL_SIZE; y++) {
      table->data[0][y] = y;
      table->data[y][y] = y;
   }

   if (callback)
      (*callback)(255);
}



/* create_color_table:
 *  Creates a color mapping table, using a user-supplied callback to blend
 *  each pair of colors. Your blend routine will be passed a pointer to the
 *  palette and the two colors to be blended (x is the source color, y is
 *  the destination), and should return the desired output RGB for this
 *  combination. If the callback function is not NULL, it will be called
 *  256 times during the calculation, allowing you to display a progress
 *  indicator.
 */
void create_color_table(COLOR_MAP *table, AL_CONST PALETTE pal, void (*blend)(AL_CONST PALETTE pal, int x, int y, RGB *rgb), void (*callback)(int pos))
{
   int x, y;
   RGB c;

   for (x=0; x<PAL_SIZE; x++) {
      for (y=0; y<PAL_SIZE; y++) {
         blend(pal, x, y, &c);

         if (rgb_map)
            table->data[x][y] = rgb_map->data[c.r>>1][c.g>>1][c.b>>1];
         else
            table->data[x][y] = bestfit_color(pal, c.r, c.g, c.b);
      }

      if (callback)
         (*callback)(x);
   }
}



/* create_blender_table:
 *  Fills the specified color mapping table with lookup data for doing a
 *  paletted equivalent of whatever truecolor blender mode is currently
 *  selected.
 */
void create_blender_table(COLOR_MAP *table, AL_CONST PALETTE pal, void (*callback)(int pos))
{
   int x, y, c;
   int r, g, b;
   int r1, g1, b1;
   int r2, g2, b2;

   ASSERT(_blender_func24);

   for (x=0; x<PAL_SIZE; x++) {
      for (y=0; y<PAL_SIZE; y++) {
         r1 = (pal[x].r << 2) | ((pal[x].r & 0x30) >> 4);
         g1 = (pal[x].g << 2) | ((pal[x].g & 0x30) >> 4);
         b1 = (pal[x].b << 2) | ((pal[x].b & 0x30) >> 4);

         r2 = (pal[y].r << 2) | ((pal[y].r & 0x30) >> 4);
         g2 = (pal[y].g << 2) | ((pal[y].g & 0x30) >> 4);
         b2 = (pal[y].b << 2) | ((pal[y].b & 0x30) >> 4);

         c = _blender_func24(makecol24(r1, g1, b1), makecol24(r2, g2, b2), _blender_alpha);

         r = getr24(c);
         g = getg24(c);
         b = getb24(c);

         if (rgb_map)
            table->data[x][y] = rgb_map->data[r>>3][g>>3][b>>3];
         else
            table->data[x][y] = bestfit_color(pal, r>>2, g>>2, b>>2);
      }

      if (callback)
         (*callback)(x);
   }
}
