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
 *      Graphics routines: palette fading, circles, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      Optimised line drawer by Michael Bukin.
 *
 *      Bresenham arc routine by Romano Signorelli.
 *
 *      Cohen-Sutherland line clipping by Jon Rafkind.
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"



/* drawing_mode:
 *  Sets the drawing mode. This only affects routines like putpixel,
 *  lines, rectangles, triangles, etc, not the blitting or sprite
 *  drawing functions.
 */
void drawing_mode(int mode, BITMAP *pattern, int x_anchor, int y_anchor)
{
   _drawing_mode = mode;
   _drawing_pattern = pattern;
   _drawing_x_anchor = x_anchor;
   _drawing_y_anchor = y_anchor;

   if (pattern) {
      _drawing_x_mask = 1; 
      while (_drawing_x_mask < (unsigned)pattern->w)
	 _drawing_x_mask <<= 1;        /* find power of two greater than w */

      if (_drawing_x_mask > (unsigned)pattern->w) {
	 ASSERT(FALSE);
	 _drawing_x_mask >>= 1;        /* round down if required */
      }

      _drawing_x_mask--;               /* convert to AND mask */

      _drawing_y_mask = 1;
      while (_drawing_y_mask < (unsigned)pattern->h)
	 _drawing_y_mask <<= 1;        /* find power of two greater than h */

      if (_drawing_y_mask > (unsigned)pattern->h) {
	 ASSERT(FALSE);
	 _drawing_y_mask >>= 1;        /* round down if required */
      }

      _drawing_y_mask--;               /* convert to AND mask */
   }
   else
      _drawing_x_mask = _drawing_y_mask = 0;

   if ((gfx_driver) && (gfx_driver->drawing_mode) && (!_dispsw_status))
      gfx_driver->drawing_mode();
}



/* set_blender_mode:
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels. The 24 bit blender is shared between the 24 and 32 bit
 *  modes. Pass a NULL table for unused color depths (you must not draw
 *  translucent graphics in modes without a handler, though!). Your blender
 *  will be passed two 32 bit colors in the appropriate format (5.5.5, 5.6.5,
 *  or 8.8.8), and an alpha value, should return the result of combining them.
 *  In translucent drawing modes, the two colors are taken from the source
 *  and destination images and the alpha is specified by this function. In
 *  lit modes, the alpha is specified when you call the drawing routine, and
 *  the interpolation is between the source color and the RGB values you pass
 *  to this function.
 */
void set_blender_mode(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b24;

   _blender_func15x = _blender_black;
   _blender_func16x = _blender_black;
   _blender_func24x = _blender_black;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* set_blender_mode_ex
 *  Specifies a custom set of blender functions for interpolating between
 *  truecolor pixels, providing a more complete set of routines, which
 *  differentiate between 24 and 32 bit modes, and have special routines
 *  for blending 32 bit RGBA pixels onto a destination of any format.
 */
void set_blender_mode_ex(BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a)
{
   _blender_func15 = b15;
   _blender_func16 = b16;
   _blender_func24 = b24;
   _blender_func32 = b32;

   _blender_func15x = b15x;
   _blender_func16x = b16x;
   _blender_func24x = b24x;

   _blender_col_15 = makecol15(r, g, b);
   _blender_col_16 = makecol16(r, g, b);
   _blender_col_24 = makecol24(r, g, b);
   _blender_col_32 = makecol32(r, g, b);

   _blender_alpha = a;
}



/* xor_mode:
 *  Shortcut function for toggling XOR mode on and off.
 */
void xor_mode(int on)
{
   drawing_mode(on ? DRAW_MODE_XOR : DRAW_MODE_SOLID, NULL, 0, 0);
}



/* solid_mode:
 *  Shortcut function for selecting solid drawing mode.
 */
void solid_mode(void)
{
   drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
}



/* clear_bitmap:
 *  Clears the bitmap to color 0.
 */
void clear_bitmap(BITMAP *bitmap)
{
   clear_to_color(bitmap, 0);
}



/* _bitmap_has_alpha:
 *  Checks whether this bitmap has an alpha channel.
 */
int _bitmap_has_alpha(BITMAP *bmp)
{
   int x, y, c;

   if (bitmap_color_depth(bmp) != 32)
      return FALSE;

   for (y = 0; y < bmp->h; y++) {
      for (x = 0; x < bmp->w; x++) {
	 c = getpixel(bmp, x, y);
	 if (geta32(c))
	    return TRUE;
      }
   }

   return FALSE;
}



/* vsync:
 *  Waits for a retrace.
 */
void vsync(void)
{
   ASSERT(gfx_driver);

   if (!_dispsw_status && gfx_driver->vsync)
      gfx_driver->vsync();
}



/* set_color:
 *  Sets a single palette entry.
 */
void set_color(int index, AL_CONST RGB *p)
{
   ASSERT(index >= 0 && index < PAL_SIZE);
   set_palette_range((struct RGB *)p-index, index, index, FALSE);
}



/* set_palette:
 *  Sets the entire color palette.
 */
void set_palette(AL_CONST PALETTE p)
{
   set_palette_range(p, 0, PAL_SIZE-1, TRUE);
}



/* set_palette_range:
 *  Sets a part of the color palette.
 */
void set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   int c;

   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE)

   for (c=from; c<=to; c++) {
      _current_palette[c] = p[c];

      if (_color_depth != 8)
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));

   if (gfx_driver) {
      if ((screen->vtable->color_depth == 8) && (!_dispsw_status))
	 gfx_driver->set_palette(p, from, to, vsync);
   }
   else if ((system_driver) && (system_driver->set_palette_range))
      system_driver->set_palette_range(p, from, to, vsync);
}



/* previous palette, so the image loaders can restore it when they are done */
int _got_prev_current_palette = FALSE;
PALETTE _prev_current_palette;
static int prev_palette_color[PAL_SIZE];



/* select_palette:
 *  Sets the aspects of the palette tables that are used for converting
 *  between different image formats, without altering the display settings.
 *  The previous settings are copied onto a one-deep stack, from where they
 *  can be restored by calling unselect_palette().
 */
void select_palette(AL_CONST PALETTE p)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      _prev_current_palette[c] = _current_palette[c];
      _current_palette[c] = p[c];
   }

   if (_color_depth != 8) {
      for (c=0; c<PAL_SIZE; c++) {
	 prev_palette_color[c] = palette_color[c];
	 palette_color[c] = makecol(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
      }
   }

   _got_prev_current_palette = TRUE;

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* unselect_palette:
 *  Restores palette settings from before the last call to select_palette().
 */
void unselect_palette(void)
{
   int c;

   for (c=0; c<PAL_SIZE; c++)
      _current_palette[c] = _prev_current_palette[c];

   if (_color_depth != 8) {
      for (c=0; c<PAL_SIZE; c++)
	 palette_color[c] = prev_palette_color[c];
   }

   ASSERT(_got_prev_current_palette == TRUE);
   _got_prev_current_palette = FALSE;

   _current_palette_changed = 0xFFFFFFFF & ~(1<<(_color_depth-1));
}



/* _palette_expansion_table:
 *  Creates a lookup table for expanding 256->truecolor.
 */
static int *palette_expansion_table(int bpp)
{
   int *table;
   int c;

   switch (bpp) {
      case 15: table = _palette_color15; break;
      case 16: table = _palette_color16; break;
      case 24: table = _palette_color24; break;
      case 32: table = _palette_color32; break;
      default: ASSERT(FALSE); return NULL;
   }

   if (_current_palette_changed & (1<<(bpp-1))) {
      for (c=0; c<PAL_SIZE; c++) {
	 table[c] = makecol_depth(bpp,
				  _rgb_scale_6[_current_palette[c].r], 
				  _rgb_scale_6[_current_palette[c].g], 
				  _rgb_scale_6[_current_palette[c].b]);
      }

      _current_palette_changed &= ~(1<<(bpp-1));
   } 

   return table;
}



/* this has to be called through a function pointer, so MSVC asm can use it */
int *(*_palette_expansion_table)(int) = palette_expansion_table;



/* generate_332_palette:
 *  Used when loading a truecolor image into an 8 bit bitmap, to generate
 *  a 3.3.2 RGB palette.
 */
void generate_332_palette(PALETTE pal)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      pal[c].r = ((c>>5)&7) * 63/7;
      pal[c].g = ((c>>2)&7) * 63/7;
      pal[c].b = (c&3) * 63/3;
   }

   pal[0].r = 63;
   pal[0].g = 0;
   pal[0].b = 63;

   pal[254].r = pal[254].g = pal[254].b = 0;
}



/* get_color:
 *  Retrieves a single color from the palette.
 */
void get_color(int index, RGB *p)
{
   ASSERT(index >= 0 && index < PAL_SIZE);
   ASSERT(p);
   get_palette_range(p-index, index, index);
}



/* get_palette:
 *  Retrieves the entire color palette.
 */
void get_palette(PALETTE p)
{
   get_palette_range(p, 0, PAL_SIZE-1);
}



/* get_palette_range:
 *  Retrieves a part of the color palette.
 */
void get_palette_range(PALETTE p, int from, int to)
{
   int c;

   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   if ((system_driver) && (system_driver->read_hardware_palette))
      system_driver->read_hardware_palette();

   for (c=from; c<=to; c++)
      p[c] = _current_palette[c];
}



/* fade_interpolate: 
 *  Calculates a palette part way between source and dest, returning it
 *  in output. The pos indicates how far between the two extremes it should
 *  be: 0 = return source, 64 = return dest, 32 = return exactly half way.
 *  Only affects colors between from and to (inclusive).
 */
void fade_interpolate(AL_CONST PALETTE source, AL_CONST PALETTE dest, PALETTE output, int pos, int from, int to)
{
   int c;

   ASSERT(pos >= 0 && pos <= 64);
   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   for (c=from; c<=to; c++) { 
      output[c].r = ((int)source[c].r * (63-pos) + (int)dest[c].r * pos) / 64;
      output[c].g = ((int)source[c].g * (63-pos) + (int)dest[c].g * pos) / 64;
      output[c].b = ((int)source[c].b * (63-pos) + (int)dest[c].b * pos) / 64;
   }
}



/* fade_from_range:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous). Only affects colors between from and to (inclusive,
 *  pass 0 and 255 to fade the entire palette).
 */
void fade_from_range(AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed, int from, int to)
{
   PALETTE temp;
   int c, start, last;

   ASSERT(speed > 0 && speed <= 64);
   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   for (c=0; c<PAL_SIZE; c++)
      temp[c] = source[c];

   if (_timer_installed) {
      start = retrace_count;
      last = -1;

      while ((c = (retrace_count-start)*speed/2) < 64) {
	 if (c != last) {
	    fade_interpolate(source, dest, temp, c, from, to);
	    set_palette_range(temp, from, to, TRUE);
	    last = c;
	 }
      }
   }
   else {
      for (c=0; c<64; c+=speed) {
	 fade_interpolate(source, dest, temp, c, from, to);
	 set_palette_range(temp, from, to, TRUE);
	 set_palette_range(temp, from, to, TRUE);
      }
   }

   set_palette_range(dest, from, to, TRUE);
}



/* fade_in_range:
 *  Fades from a solid black palette to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous). Only affects colors between from and 
 *  to (inclusive, pass 0 and 255 to fade the entire palette).
 */
void fade_in_range(AL_CONST PALETTE p, int speed, int from, int to)
{
   ASSERT(speed > 0 && speed <= 64);
   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);
   fade_from_range(black_palette, p, speed, from, to);
}



/* fade_out_range:
 *  Fades from the current palette to a solid black palette, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous). Only affects 
 *  colors between from and to (inclusive, pass 0 and 255 to fade the 
 *  entire palette).
 */
void fade_out_range(int speed, int from, int to)
{
   PALETTE temp;
   ASSERT(speed > 0 && speed <= 64);
   ASSERT(from >= 0 && from < PAL_SIZE);
   ASSERT(to >= 0 && to < PAL_SIZE);

   get_palette(temp);
   fade_from_range(temp, black_palette, speed, from, to);
}



/* fade_from:
 *  Fades from source to dest, at the specified speed (1 is the slowest, 64
 *  is instantaneous).
 */
void fade_from(AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed)
{
   ASSERT(speed > 0 && speed <= 64);
   fade_from_range(source, dest, speed, 0, PAL_SIZE-1);
}



/* fade_in:
 *  Fades from a solid black palette to p, at the specified speed (1 is
 *  the slowest, 64 is instantaneous).
 */
void fade_in(AL_CONST PALETTE p, int speed)
{
   ASSERT(speed > 0 && speed <= 64);
   fade_in_range(p, speed, 0, PAL_SIZE-1);
}



/* fade_out:
 *  Fades from the current palette to a solid black palette, at the 
 *  specified speed (1 is the slowest, 64 is instantaneous).
 */
void fade_out(int speed)
{
   ASSERT(speed > 0 && speed <= 64);
   fade_out_range(speed, 0, PAL_SIZE-1);
}



/* rect:
 *  Draws an outline rectangle.
 */
void _soft_rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   acquire_bitmap(bmp);

   hline(bmp, x1, y1, x2, color);

   if (y2 > y1)
      hline(bmp, x1, y2, x2, color);

   if (y2-1 >= y1+1) {
      vline(bmp, x1, y1+1, y2-1, color);

      if (x2 > x1)
	 vline(bmp, x2, y1+1, y2-1, color);
   }

   release_bitmap(bmp);
}



/* _normal_rectfill:
 *  Draws a solid filled rectangle, using hfill() to do the work.
 */
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int t;

   if (y1 > y2) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   if (bmp->clip) {
      if (x1 > x2) {
	 t = x1;
	 x1 = x2;
	 x2 = t;
      }

      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;

      bmp->clip = FALSE;
      t = TRUE;
   }
   else
      t = FALSE;

   acquire_bitmap(bmp);

   while (y1 <= y2) {
      bmp->vtable->hfill(bmp, x1, y1, x2, color);
      y1++;
   };

   release_bitmap(bmp);

   bmp->clip = t;
}



/* do_line:
 *  Calculates all the points along a line between x1, y1 and x2, y2, 
 *  calling the supplied function for each one. This will be passed a 
 *  copy of the bmp parameter, the x and y position, and a copy of the 
 *  d parameter (so do_line() can be used with putpixel()).
 */
void do_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int d, void (*proc)(BITMAP *, int, int, int))
{
   int dx = x2-x1;
   int dy = y2-y1;
   int i1, i2;
   int x, y;
   int dd;

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond)     \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
	 proc(bmp, x1, y1, d);                                               \
	 return;                                                             \
      }                                                                      \
									     \
      i1 = 2 * d##sec_c;                                                     \
      dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
      i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
									     \
      x = x1;                                                                \
      y = y1;                                                                \
									     \
      while (pri_c pri_cond pri_c##2) {                                      \
	 proc(bmp, x, y, d);                                                 \
									     \
	 if (dd sec_cond 0) {                                                \
	    sec_c = sec_c sec_sign 1;                                        \
	    dd += i2;                                                        \
	 }                                                                   \
	 else                                                                \
	    dd += i1;                                                        \
									     \
	 pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   }

   if (dx >= 0) {
      if (dy >= 0) {
	 if (dx >= dy) {
	    /* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, +, y, >=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, +, x, >=);
	 }
      }
      else {
	 if (dx >= -dy) {
	    /* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(+, x, <=, -, y, <=);
	 }
	 else {
	    /* (x1 <= x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, +, x, >=);
	 }
      }
   }
   else {
      if (dy >= 0) {
	 if (-dx >= dy) {
	    /* (x1 > x2) && (y1 <= y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, +, y, >=);
	 }
	 else {
	    /* (x1 > x2) && (y1 <= y2) && (dx < dy) */
	    DO_LINE(+, y, <=, -, x, <=);
	 }
      }
      else {
	 if (-dx >= -dy) {
	    /* (x1 > x2) && (y1 > y2) && (dx >= dy) */
	    DO_LINE(-, x, >=, -, y, <=);
	 }
	 else {
	    /* (x1 > x2) && (y1 > y2) && (dx < dy) */
	    DO_LINE(-, y, >=, -, x, <=);
	 }
      }
   }

   #undef DO_LINE
}



/* _normal_line:
 *  Draws a line from x1, y1 to x2, y2, using putpixel() to do the work.
 */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int sx, sy, dx, dy, t;

   if (x1 == x2) {
      vline(bmp, x1, y1, y2, color);
      return;
   }

   if (y1 == y2) {
      hline(bmp, x1, y1, x2, color);
      return;
   }

   /* use a bounding box to check if the line needs clipping */
   if (bmp->clip) {
      sx = x1;
      sy = y1;
      dx = x2;
      dy = y2;

      if (sx > dx) {
	 t = sx;
	 sx = dx;
	 dx = t;
      }

      if (sy > dy) {
	 t = sy;
	 sy = dy;
	 dy = t;
      }

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      t = TRUE;
   }
   else
      t= FALSE;

   acquire_bitmap(bmp);

   do_line(bmp, x1, y1, x2, y2, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = t;
}



/* _fast_line:
 *  Draws a line from x1, y1 to x2, y2, using putpixel() to do the work.
 *  This is an implementation of the Cohen-Sutherland line clipping algorithm.
 *  Loops over the line until it can be either trivially rejected or trivially
 *  accepted. If it is neither rejected nor accepted, subdivide it into two
 *  segments, one of which can be rejected.
 */
void _fast_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int code0, code1;
   int outcode;
   int x, y;
   int xmax, xmin, ymax, ymin;
   int done = 0, accept = 0;
   int clip_orig;
   ASSERT(bmp);

   if ((clip_orig = bmp->clip) != 0) {  /* save clipping state */
      #define TOP     0x8
      #define BOTTOM  0x4
      #define LEFT    0x2
      #define RIGHT   0x1

      #define COMPCLIP(code, x, y)  \
      {                             \
	 code = 0;                  \
	 if (y < ymin)              \
	    code |= TOP;            \
	 else if (y > ymax)         \
	    code |= BOTTOM;         \
	 if (x < xmin)              \
	    code |= LEFT;           \
	 else if (x > xmax)         \
	    code |= RIGHT;          \
      }

      xmin = bmp->cl;
      xmax = bmp->cr-1;
      ymin = bmp->ct;
      ymax = bmp->cb-1;

      COMPCLIP(code0, x1, y1);
      COMPCLIP(code1, x2, y2);

      do {

	 if (!(code0 | code1)) {
	    /* Trivially accept. */
	    accept = done = 1;
	 }
	 else if (code0 & code1) {
	    /* Trivially reject. */
	    done = 1;
	 }
	 else {
	    /* Didn't reject or accept, so do some calculations. */
	    outcode = code0 ? code0 : code1;  /* pick one endpoint */

	    if (outcode & TOP) {
	       if (y2 == y1)
		  x = x1;
	       else
		  x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
	       y = ymin;
	    }
	    else if (outcode & BOTTOM) {
	       if (y2 == y1)
		  x = x1;
	       else
		  x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
	       y = ymax;
	    }
	    else if (outcode & LEFT) {
	       if (x2 == x1)
		  y = y1;
	       else
		  y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
	       x = xmin;
	    }
	    else {  /* outcode & RIGHT */
	       if (x2 == x1)
		  y = y1;
	       else
		  y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
	       x = xmax;
	    }

	    if (outcode == code0) {
	       x1 = x;
	       y1 = y;
	       COMPCLIP(code0, x1, y1);
	    }
	    else {
	       x2 = x;
	       y2 = y;
	       COMPCLIP(code1, x2, y2);
	    }
	 }
      } while (!done);

      #undef COMPCLIP
      #undef TOP
      #undef BOTTOM
      #undef LEFT
      #undef RIGHT

      if (!accept)
	 return;

      /* We have already done the clipping, no need to do it again. */
      bmp->clip = FALSE;
   }

   if (x1 == x2) {
      bmp->vtable->vline(bmp, x1, y1, y2, color);
   }
   else if (y1 == y2) {
      bmp->vtable->hline(bmp, x1, y1, x2, color);
   }
   else {
      acquire_bitmap(bmp);
      do_line(bmp, x1, y1, x2, y2, color, bmp->vtable->putpixel);
      release_bitmap(bmp);
   }

   /* Restore original clipping state. */
   bmp->clip = clip_orig;
}



/* do_circle:
 *  Helper function for the circle drawing routines. Calculates the points
 *  in a circle of radius r around point x, y, and calls the specified 
 *  routine for each one. The output proc will be passed first a copy of
 *  the bmp parameter, then the x, y point, then a copy of the d parameter
 *  (so putpixel() can be used as the callback).
 */
void do_circle(BITMAP *bmp, int x, int y, int radius, int d, void (*proc)(BITMAP *, int, int, int))
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;

   do {
      proc(bmp, x+cx, y+cy, d); 

      if (cx) 
	 proc(bmp, x-cx, y+cy, d); 

      if (cy) 
	 proc(bmp, x+cx, y-cy, d);

      if ((cx) && (cy)) 
	 proc(bmp, x-cx, y-cy, d); 

      if (cx != cy) {
	 proc(bmp, x+cy, y+cx, d); 

	 if (cx) 
	    proc(bmp, x+cy, y-cx, d);

	 if (cy) 
	    proc(bmp, x-cy, y+cx, d); 

	 if (cx && cy) 
	    proc(bmp, x-cy, y-cx, d); 
      }

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);
}



/* circle:
 *  Draws a circle.
 */
void _soft_circle(BITMAP *bmp, int x, int y, int radius, int color)
{
   int clip, sx, sy, dx, dy;
   ASSERT(bmp);

   if (bmp->clip) {
      sx = x-radius-1;
      sy = y-radius-1;
      dx = x+radius+1;
      dy = y+radius+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do_circle(bmp, x, y, radius, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* circlefill:
 *  Draws a filled circle.
 */
void _soft_circlefill(BITMAP *bmp, int x, int y, int radius, int color)
{
   int cx = 0;
   int cy = radius;
   int df = 1 - radius; 
   int d_e = 3;
   int d_se = -2 * radius + 5;
   int clip, sx, sy, dx, dy;
   ASSERT(bmp);

   if (bmp->clip) {
      sx = x-radius-1;
      sy = y-radius-1;
      dx = x+radius+1;
      dy = y+radius+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do {
      bmp->vtable->hfill(bmp, x-cy, y-cx, x+cy, color);

      if (cx)
	 bmp->vtable->hfill(bmp, x-cy, y+cx, x+cy, color);

      if (df < 0)  {
	 df += d_e;
	 d_e += 2;
	 d_se += 2;
      }
      else { 
	 if (cx != cy) {
	    bmp->vtable->hfill(bmp, x-cx, y-cy, x+cx, color);

	    if (cy)
	       bmp->vtable->hfill(bmp, x-cx, y+cy, x+cx, color);
	 }

	 df += d_se;
	 d_e += 2;
	 d_se += 4;
	 cy--;
      } 

      cx++; 

   } while (cx <= cy);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* do_ellipse:
 *  Helper function for the ellipse drawing routines. Calculates the points
 *  in an ellipse of radius rx and ry around point x, y, and calls the 
 *  specified routine for each one. The output proc will be passed first a 
 *  copy of the bmp parameter, then the x, y point, then a copy of the d 
 *  parameter (so putpixel() can be used as the callback).
 */
void do_ellipse(BITMAP *bmp, int ix, int iy, int rx0, int ry0, int d,
   void (*proc)(BITMAP *, int, int, int))
{
   int rx, ry;
   int x, y;
   float x_change;
   float y_change;
   float ellipse_error;
   float two_a_sq;
   float two_b_sq;
   float stopping_x;
   float stopping_y;
   int midway_x = 0;

   rx = MAX(rx0, 0);
   ry = MAX(ry0, 0);

   two_a_sq = 2 * rx * rx;
   two_b_sq = 2 * ry * ry;

   x = rx;
   y = 0;

   x_change = ry * ry * (1 - 2 * rx);
   y_change = rx * rx;
   ellipse_error = 0.0;

   /* The following two variables decide when to stop.  It's easier than
    * solving for this explicitly.
    */
   stopping_x = two_b_sq * rx;
   stopping_y = 0.0;

   /* First set of points. */
   while (y <= ry) {
      proc(bmp, ix + x, iy + y, d);
      if (x != 0) {
         proc(bmp, ix - x, iy + y, d);
      }
      if (y != 0) {
         proc(bmp, ix + x, iy - y, d);
         if (x != 0) {
            proc(bmp, ix - x, iy - y, d);
         }
      }

      y++;
      stopping_y += two_a_sq;
      ellipse_error += y_change;
      y_change += two_a_sq;
      midway_x = x;

      if (stopping_x < stopping_y && x > 1) {
         break;
      }

      if ((2.0f * ellipse_error + x_change) > 0.0) {
         if (x) {
            x--;
            stopping_x -= two_b_sq;
            ellipse_error += x_change;
            x_change += two_b_sq;
         }
      }
   }

   /* To do the other half of the ellipse we reset to the top of it, and
    * iterate in the opposite direction.
    */
   x = 0;
   y = ry;

   x_change = ry * ry;
   y_change = rx * rx * (1 - 2 * ry);
   ellipse_error = 0.0;

   while (x < midway_x) {
      proc(bmp, ix + x, iy + y, d);
      if (x != 0) {
         proc(bmp, ix - x, iy + y, d);
      }
      if (y != 0) {
         proc(bmp, ix + x, iy - y, d);
         if (x != 0) {
            proc(bmp, ix - x, iy - y, d);
         }
      }

      x++;
      ellipse_error += x_change;
      x_change += two_b_sq;

      if ((2.0f * ellipse_error + y_change) > 0.0) {
         if (y) {
            y--;
            ellipse_error += y_change;
            y_change += two_a_sq;
         }
      }
   }
}



/* _soft_ellipse:
 *  Draws an ellipse.
 */
void _soft_ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int color)
{
   int clip, sx, sy, dx, dy;
   ASSERT(bmp);

   if (bmp->clip) {
      sx = x-rx-1;
      sy = y-ry-1;
      dx = x+rx+1;
      dy = y+ry+1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
	 bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   do_ellipse(bmp, x, y, rx, ry, color, bmp->vtable->putpixel);

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* _soft_ellipsefill:
 *  Draws a filled ellipse.
 */
void _soft_ellipsefill(BITMAP *bmp, int ix, int iy, int rx0, int ry0, int color)
{
   int rx, ry;
   int x, y;
   float x_change;
   float y_change;
   float ellipse_error;
   float two_a_sq;
   float two_b_sq;
   float stopping_x;
   float stopping_y;
   int clip, sx, sy, dx, dy;
   int last_drawn_y;
   int old_y;
   int midway_x = 0;
   ASSERT(bmp);

   rx = MAX(rx0, 0);
   ry = MAX(ry0, 0);

   if (bmp->clip) {
      sx = ix - rx - 1;
      sy = iy - ry - 1;
      dx = ix + rx + 1;
      dy = iy + ry + 1;

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
         return;

      if ((sx >= bmp->cl) && (sy >= bmp->ct) && (dx < bmp->cr) && (dy < bmp->cb))
         bmp->clip = FALSE;

      clip = TRUE;
   }
   else
      clip = FALSE;

   acquire_bitmap(bmp);

   two_a_sq = 2 * rx * rx;
   two_b_sq = 2 * ry * ry;

   x = rx;
   y = 0;

   x_change = ry * ry * (1 - 2 * rx);
   y_change = rx * rx;
   ellipse_error = 0.0;

   /* The following two variables decide when to stop.  It's easier than
    * solving for this explicitly.
    */
   stopping_x = two_b_sq * rx;
   stopping_y = 0.0;

   /* First set of points */
   while (y <= ry) {
      bmp->vtable->hfill(bmp, ix - x, iy + y, ix + x, color);
      if (y) {
         bmp->vtable->hfill(bmp, ix - x, iy - y, ix + x, color);
      }

      y++;
      stopping_y += two_a_sq;
      ellipse_error += y_change;
      y_change += two_a_sq;
      midway_x = x;

      if (stopping_x < stopping_y && x > 1) {
         break;
      }

      if ((2.0f * ellipse_error + x_change) > 0.0) {
         if (x) {
            x--;
            stopping_x -= two_b_sq;
            ellipse_error += x_change;
            x_change += two_b_sq;
         }
      }
   }

   last_drawn_y = y - 1;

   /* To do the other half of the ellipse we reset to the top of it, and
    * iterate in the opposite direction until we reach the place we stopped at
    * last time.
    */
   x = 0;
   y = ry;

   x_change = ry * ry;
   y_change = rx * rx * (1 - 2 * ry);
   ellipse_error = 0.0;

   old_y = y;

   while (x < midway_x) {
      if (old_y != y) {
         bmp->vtable->hfill(bmp, ix - x + 1, iy + old_y, ix + x - 1, color);
         if (old_y) {
            bmp->vtable->hfill(bmp, ix - x + 1, iy - old_y, ix + x - 1, color);
         }
      }

      x++;
      ellipse_error += x_change;
      x_change += two_b_sq;
      old_y = y;

      if ((2.0f * ellipse_error + y_change) > 0.0) {
         if (y) {
            y--;
            ellipse_error += y_change;
            y_change += two_a_sq;
         }
      }
   }

   /* On occasion, a gap appears between the middle and upper halves.
    * This 'afterthought' fills it in.
    */
   if (old_y != last_drawn_y) {
      bmp->vtable->hfill(bmp, ix - x + 1, iy + old_y, ix + x - 1, color);
      if (old_y) {
         bmp->vtable->hfill(bmp, ix - x + 1, iy - old_y, ix + x - 1, color);
      }
   }

   release_bitmap(bmp);

   bmp->clip = clip;
}



/* get_point_on_arc:
 *  Helper function for the do_arc() function, converting from (radius, angle)
 *  to (x, y).
 */
static INLINE void get_point_on_arc(int r, fixed a, int *out_x, int *out_y, int *out_q)
{
   double s, c;
   double double_a = (a&0xffffff) * (AL_PI * 2 / (1 << 24));
   s = sin(double_a);
   c = cos(double_a);
   s = -s * r;
   c = c * r;
   *out_x = (int)((c < 0) ? (c - 0.5) : (c + 0.5));
   *out_y = (int)((s < 0) ? (s - 0.5) : (s + 0.5));

   if (c >= 0) {
      if (s <= 0)
         *out_q = 0;  /* quadrant 0 */
      else
         *out_q = 3;  /* quadrant 3 */
   }
   else {
      if (s <= 0)
         *out_q = 1;  /* quadrant 1 */
      else
         *out_q = 2;  /* quadrant 2 */
   }
}



/* do_arc:
 *  Helper function for the arc function. Calculates the points in an arc
 *  of radius r around point x, y, going anticlockwise from fixed point
 *  binary angle ang1 to ang2, and calls the specified routine for each one. 
 *  The output proc will be passed first a copy of the bmp parameter, then 
 *  the x, y point, then a copy of the d parameter (so putpixel() can be 
 *  used as the callback).
 */
void do_arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int d, void (*proc)(BITMAP *, int, int, int))
{
   /* start position */
   int sx, sy;
   /* current position */
   int px, py;
   /* end position */
   int ex, ey;
   /* square of radius of circle */
   long rr;
   /* difference between main radius squared and radius squared of three
      potential next points */
   long rr1, rr2, rr3;
   /* square of x and of y */
   unsigned long xx, yy, xx_new, yy_new;
   /* start quadrant, current quadrant and end quadrant */
   int sq, q, qe;
   /* direction of movement */
   int dx, dy;
   /* temporary variable for determining if we have reached end point */
   int det;

   /* Calculate the start point and the end point. */
   /* We have to flip y because bitmaps count y coordinates downwards. */
   get_point_on_arc(r, ang1, &sx, &sy, &q);
   px = sx;
   py = sy;
   get_point_on_arc(r, ang2, &ex, &ey, &qe);

   rr = r*r;
   xx = px*px;
   yy = py*py - rr;

   sq = q;

   if (q > qe) {
      /* qe must come after q. */
      qe += 4;
   }
   else if (q == qe) {
      /* If q==qe but the beginning comes after the end, make qe be
       * strictly after q.
       */
      if (((ang2&0xffffff) < (ang1&0xffffff)) ||
	  (((ang1&0xffffff) < 0x400000) && ((ang2&0xffffff) >= 0xc00000)))
         qe += 4;
   }

   /* initial direction of movement */
   if (((q+1)&2) == 0)
      dy = -1;
   else
      dy = 1;
   if ((q&2) == 0)
      dx = -1;
   else
      dx = 1;

   while (TRUE) {
      /* Change quadrant when needed.
       * dx and dy determine the possible directions to go in this
       * quadrant, so they must be updated when we change quadrant.
       */
      if ((q&1) == 0) {
         if (px == 0) {
            if (qe == q)
	       break;
	    q++;
	    dy = -dy;
	 }
      }
      else {
         if (py == 0) {
	    if (qe == q)
	       break;
	    q++;
	    dx = -dx;
	 }
      }

      /* Are we in the final quadrant? */
      if (qe == q) {
	 /* Have we reached (or passed) the end point both in x and y? */
	 det = 0;

	 if (dy > 0) {
	    if (py >= ey)
	       det++;
	 }
	 else {
	    if (py <= ey)
	       det++;
	 }
	 if (dx > 0) {
	    if (px >= ex)
	       det++;
	 }
	 else {
	    if (px <= ex)
	       det++;
	 }
	 
	 if (det == 2)
	    break;
      }

      proc(bmp, x+px, y+py, d);

      /* From here, we have only 3 possible directions of movement, eg.
       * for the first quadrant:
       *
       *    .........
       *    .........
       *    ......21.
       *    ......3*.
       *
       * These are reached by adding dx to px and/or adding dy to py.
       * We need to find which of these points gives the best
       * approximation of the (square of the) radius.
       */

      xx_new = (px+dx) * (px+dx);
      yy_new = (py+dy) * (py+dy) - rr;
      rr1 = xx_new + yy;
      rr2 = xx_new + yy_new;
      rr3 = xx     + yy_new;

      /* Set rr1, rr2, rr3 to be the difference from the main radius of the
       * three points.
       */
      if (rr1 < 0)
	 rr1 = -rr1;
      if (rr2 < 0)
	 rr2 = -rr2;
      if (rr3 < 0)
	 rr3 = -rr3;

      if (rr3 >= MIN(rr1, rr2)) {
         px += dx;
	 xx = xx_new;
      }
      if (rr1 > MIN(rr2, rr3)) {
         py += dy;
	 yy = yy_new;
      }
   }
   /* Only draw last point if it doesn't overlap with first one. */
   if ((px != sx) || (py != sy) || (sq == qe))
      proc(bmp, x+px, y+py, d);
}



/* arc:
 *  Draws an arc.
 */
void _soft_arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color)
{
   ASSERT(bmp);
   acquire_bitmap(bmp);

   do_arc(bmp, x, y, ang1, ang2, r, color, bmp->vtable->putpixel);

   release_bitmap(bmp);
}

