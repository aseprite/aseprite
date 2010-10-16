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
 *      Sprite rotation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Flipping routines by Andrew Geers.
 *
 *      Optimized by Sven Sandberg.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include <math.h>



/*
 * Scanline drawers.
 */

#define SCANLINE_DRAWER_GENERIC(name, INIT, PUTPIXEL)              \
   static void draw_scanline_##name(BITMAP *bmp, BITMAP *spr,      \
				    fixed l_bmp_x, int bmp_y_i,    \
				    fixed r_bmp_x,                 \
				    fixed l_spr_x, fixed l_spr_y,  \
				    fixed spr_dx, fixed spr_dy)    \
   {                                                               \
      int c;                                                       \
      int mask_color;                                              \
                                                                   \
      INIT;                                                        \
      mask_color = bmp->vtable->mask_color;                        \
      r_bmp_x >>= 16;                                              \
      l_bmp_x >>= 16;                                              \
      for (; l_bmp_x <= r_bmp_x; l_bmp_x++) {                      \
	 c = getpixel(spr, l_spr_x>>16, l_spr_y>>16);              \
	 if (c != mask_color)                                      \
	    PUTPIXEL;                                              \
	 l_spr_x += spr_dx;                                        \
	 l_spr_y += spr_dy;                                        \
      }                                                            \
   }

SCANLINE_DRAWER_GENERIC(generic_convert
			,
			int bmp_depth;
			int spr_depth;
			bmp_depth = bitmap_color_depth(bmp);
			spr_depth = bitmap_color_depth(spr);
			,
			putpixel(bmp, l_bmp_x, bmp_y_i,
				 makecol_depth(bmp_depth,
					       getr_depth(spr_depth, c),
					       getg_depth(spr_depth, c),
					       getb_depth(spr_depth, c))))

SCANLINE_DRAWER_GENERIC(generic
			,
			; /* nop */
			,
			putpixel(bmp, l_bmp_x, bmp_y_i, c))



#define SCANLINE_DRAWER(bits_pp, GETPIXEL)                            \
   static void draw_scanline_##bits_pp(BITMAP *bmp, BITMAP *spr,      \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                   \
      unsigned char **spr_line = spr->line;                           \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp)                               \
	    bmp_write##bits_pp(addr, c);                              \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#ifdef ALLEGRO_COLOR8
   SCANLINE_DRAWER(8, c = spr_line[l_spr_y>>16][l_spr_x>>16]);
#endif

#ifdef ALLEGRO_COLOR16
   SCANLINE_DRAWER(15, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
   SCANLINE_DRAWER(16, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
#endif

#ifdef ALLEGRO_COLOR24
   #ifdef ALLEGRO_LITTLE_ENDIAN
      SCANLINE_DRAWER(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
						(l_spr_x>>16) * 3;
			 c = p[0];
			 c |= (int)p[1] << 8;
			 c |= (int)p[2] << 16;
		      })
   #else
      SCANLINE_DRAWER(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
					    (l_spr_x>>16) * 3;
			 c = (int)p[0] << 16;
			 c |= (int)p[1] << 8;
			 c |= p[2];
		      })
   #endif
#endif

#ifdef ALLEGRO_COLOR32
   SCANLINE_DRAWER(32,
		   c = ((uint32_t *)spr_line[l_spr_y>>16])
		       [l_spr_x>>16])
#endif

#ifdef ALLEGRO_GFX_HAS_VGA
   static void draw_scanline_modex(
    BITMAP *bmp, BITMAP *spr, fixed l_bmp_x, int bmp_y_i, fixed r_bmp_x,
    fixed l_spr_x, fixed l_spr_y, fixed spr_dx, fixed spr_dy)
   {
      int c;
      uintptr_t start_addr, addr, end_addr;
      unsigned char **spr_line = spr->line;
      int plane;
      fixed spr_x, spr_y;

      r_bmp_x >>= 16;
      l_bmp_x >>= 16;
      start_addr = (uintptr_t)bmp->line[bmp_y_i];
      spr_dx <<= 2;
      spr_dy <<= 2;
      for (plane = 0; plane < 4; plane++) {
	 addr = start_addr + ((l_bmp_x + plane) >> 2);
	 end_addr = addr + ((r_bmp_x - l_bmp_x - plane) >> 2);
	 outportw(0x3C4, (0x100 << ((l_bmp_x + plane) & 3)) | 2);
	 spr_x = l_spr_x;
	 spr_y = l_spr_y;
	 for (; addr < end_addr; addr++) {
	    c = spr_line[spr_y >> 16][spr_x >> 16];
	    if (c != MASK_COLOR_8)
	       _farnspokeb(addr, c);
	    spr_x += spr_dx;
	    spr_y += spr_dy;
	 }
	 l_spr_x += spr_dx >> 2;
	 l_spr_y += spr_dy >> 2;
      }
   }
#endif



/* _parallelogram_map:
 *  Worker routine for drawing rotated and/or scaled and/or flipped sprites:
 *  It actually maps the sprite to any parallelogram-shaped area of the
 *  bitmap. The top left corner is mapped to (xs[0], ys[0]), the top right to
 *  (xs[1], ys[1]), the bottom right to x (xs[2], ys[2]), and the bottom left
 *  to (xs[3], ys[3]). The corners are assumed to form a perfect
 *  parallelogram, i.e. xs[0]+xs[2] = xs[1]+xs[3]. The corners are given in
 *  fixed point format, so xs[] and ys[] are coordinates of the outer corners
 *  of corner pixels in clockwise order beginning with top left.
 *  All coordinates begin with 0 in top left corner of pixel (0, 0). So a
 *  rotation by 0 degrees of a sprite to the top left of a bitmap can be
 *  specified with coordinates (0, 0) for the top left pixel in source
 *  bitmap. With the default scanline drawer, a pixel in the destination
 *  bitmap is drawn if and only if its center is covered by any pixel in the
 *  sprite. The color of this covering sprite pixel is used to draw.
 *  If sub_pixel_accuracy=FALSE, then the scanline drawer will be called with
 *  *_bmp_x being a fixed point representation of the integers representing
 *  the x coordinate of the first and last point in bmp whose centre is
 *  covered by the sprite. If sub_pixel_accuracy=TRUE, then the scanline
 *  drawer will be called with the exact fixed point position of the first
 *  and last point in which the horizontal line passing through the centre is
 *  at least partly covered by the sprite. This is useful for doing
 *  anti-aliased blending.
 */
void _parallelogram_map(BITMAP *bmp, BITMAP *spr, fixed xs[4], fixed ys[4],
			void (*draw_scanline)(BITMAP *bmp, BITMAP *spr,
					      fixed l_bmp_x, int bmp_y,
					      fixed r_bmp_x,
					      fixed l_spr_x, fixed l_spr_y,
					      fixed spr_dx, fixed spr_dy),
			int sub_pixel_accuracy)
{
   /* Index in xs[] and ys[] to topmost point. */
   int top_index;
   /* Rightmost point has index (top_index+right_index) int xs[] and ys[]. */
   int right_index;
   /* Loop variables. */
   int index, i;
   /* Coordinates in bmp ordered as top-right-bottom-left. */
   fixed corner_bmp_x[4], corner_bmp_y[4];
   /* Coordinates in spr ordered as top-right-bottom-left. */
   fixed corner_spr_x[4], corner_spr_y[4];
   /* y coordinate of bottom point, left point and right point. */
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i;
   /* Left and right clipping. */
   fixed clip_left, clip_right;
   /* Temporary variable. */
   fixed extra_scanline_fraction;

   /*
    * Variables used in the loop
    */
   /* Coordinates of sprite and bmp points in beginning of scanline. */
   fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx;
   /* Increment of left sprite point as we move a scanline down. */
   fixed l_spr_dx, l_spr_dy;
   /* Coordinates of sprite and bmp points in end of scanline. */
   fixed r_bmp_x, r_bmp_dx;
   #ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE
   fixed r_spr_x, r_spr_y;
   /* Increment of right sprite point as we move a scanline down. */
   fixed r_spr_dx, r_spr_dy;
   #endif
   /* Increment of sprite point as we move right inside a scanline. */
   fixed spr_dx, spr_dy;
   /* Positions of beginning of scanline after rounding to integer coordinate
      in bmp. */
   fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded;
   fixed r_bmp_x_rounded;
   /* Current scanline. */
   int bmp_y_i;
   /* Right edge of scanline. */
   int right_edge_test;

   /* Get index of topmost point. */
   top_index = 0;
   if (ys[1] < ys[0])
      top_index = 1;
   if (ys[2] < ys[top_index])
      top_index = 2;
   if (ys[3] < ys[top_index])
      top_index = 3;

   /* Get direction of points: clockwise or anti-clockwise. */
   right_index = (double)(xs[(top_index+1) & 3] - xs[top_index]) *
      (double)(ys[(top_index-1) & 3] - ys[top_index]) >
      (double)(xs[(top_index-1) & 3] - xs[top_index]) *
      (double)(ys[(top_index+1) & 3] - ys[top_index]) ? 1 : -1;
   //FIXME: why does fixmul overflow below?
   /*if (fixmul(xs[(top_index+1) & 3] - xs[top_index],
      ys[(top_index-1) & 3] - ys[top_index]) >
         fixmul(xs[(top_index-1) & 3] - xs[top_index],
            ys[(top_index+1) & 3] - ys[top_index]))
      right_index = 1;
   else
      right_index = -1;*/

   /*
    * Get coordinates of the corners.
    */

   /* corner_*[0] is top, [1] is right, [2] is bottom, [3] is left. */
   index = top_index;
   for (i = 0; i < 4; i++) {
      corner_bmp_x[i] = xs[index];
      corner_bmp_y[i] = ys[index];
      if (index < 2)
	 corner_spr_y[i] = 0;
      else
	 /* Need `- 1' since otherwise it would be outside sprite. */
	 corner_spr_y[i] = (spr->h << 16) - 1;
      if ((index == 0) || (index == 3))
	 corner_spr_x[i] = 0;
      else
	 corner_spr_x[i] = (spr->w << 16) - 1;
      index = (index + right_index) & 3;
   }

   /*
    * Get scanline starts, ends and deltas, and clipping coordinates.
    */
   #define top_bmp_y    corner_bmp_y[0]
   #define right_bmp_y  corner_bmp_y[1]
   #define bottom_bmp_y corner_bmp_y[2]
   #define left_bmp_y   corner_bmp_y[3]
   #define top_bmp_x    corner_bmp_x[0]
   #define right_bmp_x  corner_bmp_x[1]
   #define bottom_bmp_x corner_bmp_x[2]
   #define left_bmp_x   corner_bmp_x[3]
   #define top_spr_y    corner_spr_y[0]
   #define right_spr_y  corner_spr_y[1]
   #define bottom_spr_y corner_spr_y[2]
   #define left_spr_y   corner_spr_y[3]
   #define top_spr_x    corner_spr_x[0]
   #define right_spr_x  corner_spr_x[1]
   #define bottom_spr_x corner_spr_x[2]
   #define left_spr_x   corner_spr_x[3]

   /* Calculate left and right clipping. */
   if (bmp->clip) {
      clip_left = bmp->cl << 16;
      clip_right = (bmp->cr << 16) - 1;
   }
   else {
      ASSERT(left_bmp_x >= 0 && top_bmp_x >= 0 && bottom_bmp_x >= 0
	     && right_bmp_x < (bmp->w << 16)
	     && top_bmp_x < (bmp->w << 16)
	     && bottom_bmp_x < (bmp->w << 16));
      clip_left = 0;
      clip_right = (bmp->w << 16) - 1;
   }

   /* Quit if we're totally outside. */
   if ((left_bmp_x > clip_right) &&
       (top_bmp_x > clip_right) &&
       (bottom_bmp_x > clip_right))
      return;
   if ((right_bmp_x < clip_left) &&
       (top_bmp_x < clip_left) &&
       (bottom_bmp_x < clip_left))
      return;

   /* Bottom clipping. */
   if (sub_pixel_accuracy)
      clip_bottom_i = (bottom_bmp_y + 0xffff) >> 16;
   else
      clip_bottom_i = (bottom_bmp_y + 0x8000) >> 16;
   if (bmp->clip) {
      if (clip_bottom_i > bmp->cb)
	 clip_bottom_i = bmp->cb;
   }
   else {
      ASSERT(clip_bottom_i <= bmp->h);
   }

   /* Calculate y coordinate of first scanline. */
   if (sub_pixel_accuracy)
      bmp_y_i = top_bmp_y >> 16;
   else
      bmp_y_i = (top_bmp_y + 0x8000) >> 16;
   if (bmp->clip) {
      if (bmp_y_i < bmp->ct)
	 bmp_y_i = bmp->ct;
   }
   else {
      ASSERT(bmp_y_i >= 0);
   }

   /* Sprite is above or below bottom clipping area. */
   if (bmp_y_i >= clip_bottom_i)
      return;

   /* Vertical gap between top corner and centre of topmost scanline. */
   extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - top_bmp_y;
   /* Calculate x coordinate of beginning of scanline in bmp. */
   l_bmp_dx = fixdiv(left_bmp_x - top_bmp_x,
		   left_bmp_y - top_bmp_y);
   l_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);
   /* Calculate x coordinate of beginning of scanline in spr. */
   /* note: all these are rounded down which is probably a Good Thing (tm) */
   l_spr_dx = fixdiv(left_spr_x - top_spr_x,
		   left_bmp_y - top_bmp_y);
   l_spr_x = top_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);
   /* Calculate y coordinate of beginning of scanline in spr. */
   l_spr_dy = fixdiv(left_spr_y - top_spr_y,
		   left_bmp_y - top_bmp_y);
   l_spr_y = top_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);

   /* Calculate left loop bound. */
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16;
   if (l_bmp_y_bottom_i > clip_bottom_i)
      l_bmp_y_bottom_i = clip_bottom_i;

   /* Calculate x coordinate of end of scanline in bmp. */
   r_bmp_dx = fixdiv(right_bmp_x - top_bmp_x,
		   right_bmp_y - top_bmp_y);
   r_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);
   #ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE
   /* Calculate x coordinate of end of scanline in spr. */
   r_spr_dx = fixdiv(right_spr_x - top_spr_x,
		   right_bmp_y - top_bmp_y);
   r_spr_x = top_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);
   /* Calculate y coordinate of end of scanline in spr. */
   r_spr_dy = fixdiv(right_spr_y - top_spr_y,
		   right_bmp_y - top_bmp_y);
   r_spr_y = top_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);
   #endif

   /* Calculate right loop bound. */
   r_bmp_y_bottom_i = (right_bmp_y + 0x8000) >> 16;

   /* Get dx and dy, the offsets to add to the source coordinates as we move
      one pixel rightwards along a scanline. This formula can be derived by
      considering the 2x2 matrix that transforms the sprite to the
      parallelogram.
      We'd better use double to get this as exact as possible, since any
      errors will be accumulated along the scanline.
   */
   spr_dx = (fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * spr->w) /
		    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) -
		     (xs[3] - xs[0]) * (double)(ys[1] - ys[0])));
   spr_dy = (fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * spr->h) /
		    ((xs[3] - xs[0]) * (double)(ys[1] - ys[0]) -
		     (xs[1] - xs[0]) * (double)(ys[3] - ys[0])));

   /*
    * Loop through scanlines.
    */

   while (1) {
      /* Has beginning of scanline passed a corner? */
      if (bmp_y_i >= l_bmp_y_bottom_i) {
	 /* Are we done? */
	 if (bmp_y_i >= clip_bottom_i)
	    break;

	 /* Vertical gap between left corner and centre of scanline. */
	 extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - left_bmp_y;
	 /* Update x coordinate of beginning of scanline in bmp. */
	 l_bmp_dx = fixdiv(bottom_bmp_x - left_bmp_x,
			 bottom_bmp_y - left_bmp_y);
	 l_bmp_x = left_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);
	 /* Update x coordinate of beginning of scanline in spr. */
	 l_spr_dx = fixdiv(bottom_spr_x - left_spr_x,
			 bottom_bmp_y - left_bmp_y);
	 l_spr_x = left_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);
	 /* Update y coordinate of beginning of scanline in spr. */
	 l_spr_dy = fixdiv(bottom_spr_y - left_spr_y,
			 bottom_bmp_y - left_bmp_y);
	 l_spr_y = left_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);

	 /* Update loop bound. */
	 if (sub_pixel_accuracy)
	    l_bmp_y_bottom_i = (bottom_bmp_y + 0xffff) >> 16;
	 else
	    l_bmp_y_bottom_i = (bottom_bmp_y + 0x8000) >> 16;
	 if (l_bmp_y_bottom_i > clip_bottom_i)
	    l_bmp_y_bottom_i = clip_bottom_i;
      }

      /* Has end of scanline passed a corner? */
      if (bmp_y_i >= r_bmp_y_bottom_i) {
	 /* Vertical gap between right corner and centre of scanline. */
	 extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - right_bmp_y;
	 /* Update x coordinate of end of scanline in bmp. */
	 r_bmp_dx = fixdiv(bottom_bmp_x - right_bmp_x,
			 bottom_bmp_y - right_bmp_y);
	 r_bmp_x = right_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);
	 #ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE
	 /* Update x coordinate of beginning of scanline in spr. */
	 r_spr_dx = fixdiv(bottom_spr_x - right_spr_x,
			 bottom_bmp_y - right_bmp_y);
	 r_spr_x = right_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);
	 /* Update y coordinate of beginning of scanline in spr. */
	 r_spr_dy = fixdiv(bottom_spr_y - right_spr_y,
			 bottom_bmp_y - right_bmp_y);
	 r_spr_y = right_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);
	 #endif

	 /* Update loop bound: We aren't supposed to use this any more, so
	    just set it to some big enough value. */
	 r_bmp_y_bottom_i = clip_bottom_i;
      }

      /* Make left bmp coordinate be an integer and clip it. */
      if (sub_pixel_accuracy)
	 l_bmp_x_rounded = l_bmp_x;
      else
	 l_bmp_x_rounded = (l_bmp_x + 0x8000) & ~0xffff;
      if (l_bmp_x_rounded < clip_left)
	 l_bmp_x_rounded = clip_left;

      /* ... and move starting point in sprite accordingly. */
      if (sub_pixel_accuracy) {
	 l_spr_x_rounded = l_spr_x +
			   fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx);
	 l_spr_y_rounded = l_spr_y +
			   fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy);
      }
      else {
	 l_spr_x_rounded = l_spr_x +
			   fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx);
	 l_spr_y_rounded = l_spr_y +
			   fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy);
      }

      /* Make right bmp coordinate be an integer and clip it. */
      if (sub_pixel_accuracy)
	 r_bmp_x_rounded = r_bmp_x;
      else
	 r_bmp_x_rounded = (r_bmp_x - 0x8000) & ~0xffff;
      if (r_bmp_x_rounded > clip_right)
	 r_bmp_x_rounded = clip_right;

      /* Draw! */
      if (l_bmp_x_rounded <= r_bmp_x_rounded) {
	 if (!sub_pixel_accuracy) {
	    /* The bodies of these ifs are only reached extremely seldom,
	       it's an ugly hack to avoid reading outside the sprite when
	       the rounding errors are accumulated the wrong way. It would
	       be nicer if we could ensure that this never happens by making
	       all multiplications and divisions be rounded up or down at
	       the correct places.
	       I did try another approach: recalculate the edges of the
	       scanline from scratch each scanline rather than incrementally.
	       Drawing a sprite with that routine took about 25% longer time
	       though.
	    */
	    if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)spr->w) {
	       if (((l_spr_x_rounded < 0) && (spr_dx <= 0)) ||
		   ((l_spr_x_rounded > 0) && (spr_dx >= 0))) {
		  /* This can happen. */
		  goto skip_draw;
	       }
	       else {
		  /* I don't think this can happen, but I can't prove it. */
		  do {
		     l_spr_x_rounded += spr_dx;
		     l_bmp_x_rounded += 65536;
		     if (l_bmp_x_rounded > r_bmp_x_rounded)
			goto skip_draw;
		  } while ((unsigned)(l_spr_x_rounded >> 16) >=
			   (unsigned)spr->w);

	       }
	    }
	    right_edge_test = l_spr_x_rounded +
			      ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
			      spr_dx;
	    if ((unsigned)(right_edge_test >> 16) >= (unsigned)spr->w) {
	       if (((right_edge_test < 0) && (spr_dx <= 0)) ||
		   ((right_edge_test > 0) && (spr_dx >= 0))) {
		  /* This can happen. */
		  do {
		     r_bmp_x_rounded -= 65536;
		     right_edge_test -= spr_dx;
		     if (l_bmp_x_rounded > r_bmp_x_rounded)
			goto skip_draw;
		  } while ((unsigned)(right_edge_test >> 16) >=
			   (unsigned)spr->w);
	       }
	       else {
		  /* I don't think this can happen, but I can't prove it. */
		  goto skip_draw;
	       }
	    }
	    if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)spr->h) {
	       if (((l_spr_y_rounded < 0) && (spr_dy <= 0)) ||
		   ((l_spr_y_rounded > 0) && (spr_dy >= 0))) {
		  /* This can happen. */
		  goto skip_draw;
	       }
	       else {
		  /* I don't think this can happen, but I can't prove it. */
		  do {
		     l_spr_y_rounded += spr_dy;
		     l_bmp_x_rounded += 65536;
		     if (l_bmp_x_rounded > r_bmp_x_rounded)
			goto skip_draw;
		  } while (((unsigned)l_spr_y_rounded >> 16) >=
			   (unsigned)spr->h);
	       }
	    }
	    right_edge_test = l_spr_y_rounded +
			      ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
			      spr_dy;
	    if ((unsigned)(right_edge_test >> 16) >= (unsigned)spr->h) {
	       if (((right_edge_test < 0) && (spr_dy <= 0)) ||
		   ((right_edge_test > 0) && (spr_dy >= 0))) {
		  /* This can happen. */
		  do {
		     r_bmp_x_rounded -= 65536;
		     right_edge_test -= spr_dy;
		     if (l_bmp_x_rounded > r_bmp_x_rounded)
			goto skip_draw;
		  } while ((unsigned)(right_edge_test >> 16) >=
			   (unsigned)spr->h);
	       }
	       else {
		  /* I don't think this can happen, but I can't prove it. */
		  goto skip_draw;
	       }
	    }
	 }
	 draw_scanline(bmp, spr,
		       l_bmp_x_rounded, bmp_y_i, r_bmp_x_rounded,
		       l_spr_x_rounded, l_spr_y_rounded,
		       spr_dx, spr_dy);

      }
      /* I'm not going to apoligize for this label and its gotos: to get
         rid of it would just make the code look worse. */
      skip_draw:

      /* Jump to next scanline. */
      bmp_y_i++;
      /* Update beginning of scanline. */
      l_bmp_x += l_bmp_dx;
      l_spr_x += l_spr_dx;
      l_spr_y += l_spr_dy;
      /* Update end of scanline. */
      r_bmp_x += r_bmp_dx;
      #ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE
      r_spr_x += r_spr_dx;
      r_spr_y += r_spr_dy;
      #endif
   }

   bmp_unwrite_line(bmp);
}



/* _parallelogram_map_standard:
 *  Helper function for calling _parallelogram_map() with the appropriate
 *  scanline drawer. I didn't want to include this in the
 *  _parallelogram_map() function since then you can bypass it and define
 *  your own scanline drawer, eg. for anti-aliased rotations.
 */
void _parallelogram_map_standard(BITMAP *bmp, BITMAP *sprite,
				 fixed xs[4], fixed ys[4])
{
   int old_drawing_mode;
   if (bitmap_color_depth(bmp) != bitmap_color_depth(sprite)) {
      /* These scanline drawers use putpixel() so we must set solid mode. */
      old_drawing_mode = _drawing_mode;
      drawing_mode(DRAW_MODE_SOLID, _drawing_pattern,
		   _drawing_x_anchor, _drawing_y_anchor);
      _parallelogram_map(bmp, sprite, xs, ys,
			 draw_scanline_generic_convert, FALSE);
      drawing_mode(old_drawing_mode, _drawing_pattern,
		   _drawing_x_anchor, _drawing_y_anchor);
   }
   else if (!is_memory_bitmap(sprite)) {
      old_drawing_mode = _drawing_mode;
      drawing_mode(DRAW_MODE_SOLID, _drawing_pattern,
		   _drawing_x_anchor, _drawing_y_anchor);
      _parallelogram_map(bmp, sprite, xs, ys,
			 draw_scanline_generic, FALSE);
      drawing_mode(old_drawing_mode, _drawing_pattern,
		   _drawing_x_anchor, _drawing_y_anchor);
   }
   else if (is_linear_bitmap(bmp)) {
      switch (bitmap_color_depth(bmp)) {
	 #ifdef ALLEGRO_COLOR8
	    case 8:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_8, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR16
	    case 15:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_15, FALSE);
	       break;

	    case 16:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_16, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR24
	    case 24:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_24, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR32
	    case 32:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_32, FALSE);
	       break;
	 #endif

	 default:
	    /* NOTREACHED */
	    ASSERT(0);
      }
   }
   #ifdef ALLEGRO_GFX_HAS_VGA
      else {
	 _parallelogram_map(bmp, sprite, xs, ys,
			    draw_scanline_modex, FALSE);
      }
   #endif
}



/* _rotate_scale_flip_coordinates:
 *  Calculates the coordinates for the rotated, scaled and flipped sprite,
 *  and passes them on to the given function.
 */
void _rotate_scale_flip_coordinates(fixed w, fixed h,
				    fixed x, fixed y, fixed cx, fixed cy,
				    fixed angle,
				    fixed scale_x, fixed scale_y,
				    int h_flip, int v_flip,
				    fixed xs[4], fixed ys[4])
{
   fixed fix_cos, fix_sin;
   int tl = 0, tr = 1, bl = 3, br = 2;
   int tmp;
   double cos_angle, sin_angle;
   fixed xofs, yofs;

   /* Setting angle to the range -180...180 degrees makes sin & cos
      more numerically stable. (Yes, this does have an effect for big
      angles!) Note that using "real" sin() and cos() gives much better
      precision than fixsin() and fixcos(). */
   angle = angle & 0xffffff;
   if (angle >= 0x800000)
      angle -= 0x1000000;

   _AL_SINCOS(angle * (AL_PI / (double)0x800000), sin_angle, cos_angle);

   if (cos_angle >= 0)
      fix_cos = (int)(cos_angle * 0x10000 + 0.5);
   else
      fix_cos = (int)(cos_angle * 0x10000 - 0.5);
   if (sin_angle >= 0)
      fix_sin = (int)(sin_angle * 0x10000 + 0.5);
   else
      fix_sin = (int)(sin_angle * 0x10000 - 0.5);

   /* Decide what order to take corners in. */
   if (v_flip) {
      tl = 3;
      tr = 2;
      bl = 0;
      br = 1;
   }
   else {
      tl = 0;
      tr = 1;
      bl = 3;
      br = 2;
   }
   if (h_flip) {
      tmp = tl;
      tl = tr;
      tr = tmp;
      tmp = bl;
      bl = br;
      br = tmp;
   }

   /* Calculate new coordinates of all corners. */
   w = fixmul(w, scale_x);
   h = fixmul(h, scale_y);
   cx = fixmul(cx, scale_x);
   cy = fixmul(cy, scale_y);

   xofs = x - fixmul(cx, fix_cos) + fixmul(cy, fix_sin);

   yofs = y - fixmul(cx, fix_sin) - fixmul(cy, fix_cos);

   xs[tl] = xofs;
   ys[tl] = yofs;
   xs[tr] = xofs + fixmul(w, fix_cos);
   ys[tr] = yofs + fixmul(w, fix_sin);
   xs[bl] = xofs - fixmul(h, fix_sin);
   ys[bl] = yofs + fixmul(h, fix_cos);

   xs[br] = xs[tr] + xs[bl] - xs[tl];
   ys[br] = ys[tr] + ys[bl] - ys[tl];
}



/* _pivot_scaled_sprite_flip:
 *  The most generic routine to which you specify the position with angles,
 *  scales, etc.
 */
void _pivot_scaled_sprite_flip(BITMAP *bmp, BITMAP *sprite,
			       fixed x, fixed y, fixed cx, fixed cy,
			       fixed angle, fixed scale, int v_flip)
{
   fixed xs[4], ys[4];

   _rotate_scale_flip_coordinates(sprite->w << 16, sprite->h << 16,
				  x, y, cx, cy, angle, scale, scale,
				  FALSE, v_flip, xs, ys);

   _parallelogram_map_standard(bmp, sprite, xs, ys);
}



/*
 * rotate_*()
 */



/* _rotate_scaled_sprite_flip: (template)
 *  Rotates and scales a sprite, optionally flipping it about either axis.
 *  Coordinates are given in fixed point format.
 */
#if 0
static void _rotate_scaled_sprite_flip(BITMAP *bmp, BITMAP *sprite,
				          fixed x, fixed y,
				          fixed angle, fixed scale, int v_flip)
{
   _pivot_scaled_sprite_flip(bmp, sprite,
			     x + (sprite->w * scale) / 2,
			     y + (sprite->h * scale) / 2,
			     sprite->w << 15, sprite->h << 15,
			     angle, scale, v_flip);
}
#endif



/* rotate_sprite: (inlined)
 *  Draws a sprite image onto a bitmap at the specified position, rotating
 *  it by the specified angle. The angle is a fixed point 16.16 number in
 *  the same format used by the fixed point trig routines, with 256 equal
 *  to a full circle, 64 a right angle, etc. This function can draw between
 *  any two bitmaps.
 */



/* rotate_sprite_v_flip: (inlined)
 *  Similar to rotate_sprite(), except flips the sprite vertically first.
 */



/* rotate_scaled_sprite: (inlined)
 *  Draws a sprite image onto a bitmap at the specified position, rotating
 *  it by the specified angle. The angle is a fixed point 16.16 number in
 *  the same format used by the fixed point trig routines, with 256 equal
 *  to a full circle, 64 a right angle, etc. This function can draw between
 *  any two bitmaps.
 */



/* rotate_scaled_sprite_v_flip: (inlined)
 *  Similar to rotate_scaled_sprite(), except flips the sprite vertically first.
 */



/*
 * pivot_*()
 */



/* pivot_sprite: (inlined)
 *  Rotates a sprite around the specified pivot centre point.
 */



/* pivot_sprite_v_flip: (inlined)
 *  Similar to pivot_sprite(), except flips the sprite vertically first.
 */



/* pivot_scaled_sprite: (inlined)
 *  Rotates and scales a sprite around the specified pivot centre point.
 */



/* pivot_scaled_sprite_v_flip: (inlined)
 *  Similar to pivot_scaled_sprite(), except flips the sprite vertically first.
 */
