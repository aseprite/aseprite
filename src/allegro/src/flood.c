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
 *      The floodfill routine.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct FLOODED_LINE      /* store segments which have been flooded */
{
   short flags;                  /* status of the segment */
   short lpos, rpos;             /* left and right ends of segment */
   short y;                      /* y coordinate of the segment */
   int next;                     /* linked list if several per line */
} FLOODED_LINE;

/* Note: a 'short' is not sufficient for 'next' above in some corner cases. */


static int flood_count;          /* number of flooded segments */

#define FLOOD_IN_USE             1
#define FLOOD_TODO_ABOVE         2
#define FLOOD_TODO_BELOW         4

#define FLOOD_LINE(c)            (((FLOODED_LINE *)_scratch_mem) + c)



/* flooder:
 *  Fills a horizontal line around the specified position, and adds it
 *  to the list of drawn segments. Returns the first x coordinate after 
 *  the part of the line which it has dealt with.
 */
static int flooder(BITMAP *bmp, int x, int y, int src_color, int dest_color)
{
   FLOODED_LINE *p;
   int left = 0, right = 0;
   unsigned long addr;
   int c;

   /* helper for doing checks in each color depth */
   #define FLOODER(bits, size)                                               \
   {                                                                         \
      /* check start pixel */                                                \
      if ((int)bmp_read##bits(addr+x*size) != src_color)                     \
	 return x+1;                                                         \
									     \
      /* work left from starting point */                                    \
      for (left=x-1; left>=bmp->cl; left--) {                                \
	 if ((int)bmp_read##bits(addr+left*size) != src_color)               \
	    break;                                                           \
      }                                                                      \
									     \
      /* work right from starting point */                                   \
      for (right=x+1; right<bmp->cr; right++) {                              \
	 if ((int)bmp_read##bits(addr+right*size) != src_color)              \
	    break;                                                           \
      }                                                                      \
   }

   ASSERT(bmp);
   
   if (is_linear_bitmap(bmp)) {     /* use direct access for linear bitmaps */
      addr = bmp_read_line(bmp, y);
      bmp_select(bmp);

      switch (bitmap_color_depth(bmp)) {

	 #ifdef ALLEGRO_COLOR8
	    case 8:
	       FLOODER(8, 1);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR16
	    case 15:
	    case 16:
	       FLOODER(16, sizeof(short));
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR24
	    case 24:
	       FLOODER(24, 3);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR32
	    case 32:
	       FLOODER(32, sizeof(int32_t));
	       break;
	 #endif
      }

      bmp_unwrite_line(bmp);
   }
   else {                           /* have to use getpixel() for mode-X */
      /* check start pixel */
      if (getpixel(bmp, x, y) != src_color)
	 return x+1;

      /* work left from starting point */ 
      for (left=x-1; left>=bmp->cl; left--)
	 if (getpixel(bmp, left, y) != src_color)
	    break;

      /* work right from starting point */ 
      for (right=x+1; right<bmp->cr; right++)
	 if (getpixel(bmp, right, y) != src_color)
	    break;
   } 

   left++;
   right--;

   /* draw the line */
   bmp->vtable->hfill(bmp, left, y, right, dest_color);

   /* store it in the list of flooded segments */
   c = y;
   p = FLOOD_LINE(c);

   if (p->flags) {
      while (p->next) {
	 c = p->next;
	 p = FLOOD_LINE(c);
      }

      p->next = c = flood_count++;
      _grow_scratch_mem(sizeof(FLOODED_LINE) * flood_count);
      p = FLOOD_LINE(c);
   }

   p->flags = FLOOD_IN_USE;
   p->lpos = left;
   p->rpos = right;
   p->y = y;
   p->next = 0;

   if (y > bmp->ct)
      p->flags |= FLOOD_TODO_ABOVE;

   if (y+1 < bmp->cb)
      p->flags |= FLOOD_TODO_BELOW;

   return right+2;
}



/* check_flood_line:
 *  Checks a line segment, using the scratch buffer is to store a list of 
 *  segments which have already been drawn in order to minimise the required 
 *  number of tests.
 */
static int check_flood_line(BITMAP *bmp, int y, int left, int right, int src_color, int dest_color)
{
   int c;
   FLOODED_LINE *p;
   int ret = FALSE;

   while (left <= right) {
      c = y;

      for (;;) {
	 p = FLOOD_LINE(c);

	 if ((left >= p->lpos) && (left <= p->rpos)) {
	    left = p->rpos+2;
	    break;
	 }

	 c = p->next;

	 if (!c) {
	    left = flooder(bmp, left, y, src_color, dest_color);
	    ret = TRUE;
	    break; 
	 }
      }
   }

   return ret;
}



/* floodfill:
 *  Fills an enclosed area (starting at point x, y) with the specified color.
 */
void _soft_floodfill(BITMAP *bmp, int x, int y, int color)
{
   int src_color;
   int c, done;
   FLOODED_LINE *p;
   ASSERT(bmp);

   /* make sure we have a valid starting point */ 
   if ((x < bmp->cl) || (x >= bmp->cr) || (y < bmp->ct) || (y >= bmp->cb))
      return;

   acquire_bitmap(bmp);

   /* what color to replace? */
   src_color = getpixel(bmp, x, y);
   if (src_color == color) {
      release_bitmap(bmp);
      return;
   }

   /* set up the list of flooded segments */
   _grow_scratch_mem(sizeof(FLOODED_LINE) * bmp->cb);
   flood_count = bmp->cb;
   p = _scratch_mem;
   for (c=0; c<flood_count; c++) {
      p[c].flags = 0;
      p[c].lpos = SHRT_MAX;
      p[c].rpos = SHRT_MIN;
      p[c].y = y;
      p[c].next = 0;
   }

   /* start up the flood algorithm */
   flooder(bmp, x, y, src_color, color);

   /* continue as long as there are some segments still to test */
   do {
      done = TRUE;

      /* for each line on the screen */
      for (c=0; c<flood_count; c++) {

	 p = FLOOD_LINE(c);

	 /* check below the segment? */
	 if (p->flags & FLOOD_TODO_BELOW) {
	    p->flags &= ~FLOOD_TODO_BELOW;
	    if (check_flood_line(bmp, p->y+1, p->lpos, p->rpos, src_color, color)) {
	       done = FALSE;
	       p = FLOOD_LINE(c);
	    }
	 }

	 /* check above the segment? */
	 if (p->flags & FLOOD_TODO_ABOVE) {
	    p->flags &= ~FLOOD_TODO_ABOVE;
	    if (check_flood_line(bmp, p->y-1, p->lpos, p->rpos, src_color, color)) {
	       done = FALSE;
	       /* special case shortcut for going backwards */
	       if ((c < bmp->cb) && (c > 0))
		  c -= 2;
	    }
	 }
      }

   } while (!done);

   release_bitmap(bmp);
}

