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
 *      Gouraud shaded sprite renderer.
 *
 *      By Patrick Hogan.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* draw_gouraud_sprite:
 *  Draws a lit or tinted sprite, interpolating the four corner colors
 *  over the surface of the image.
 */
void _soft_draw_gouraud_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4)
{
   fixed mc1, mc2, mh;
   fixed lc, rc, hc;
   int x1 = x;
   int y1 = y;
   int x2 = x + sprite->w;
   int y2 = y + sprite->h;
   int i, j;
   int pixel;
   uintptr_t addr;

   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp_select(bmp);

   /* set up vertical gradients for left and right sides */
   mc1 = itofix(c4 - c1) / sprite->h;
   mc2 = itofix(c3 - c2) / sprite->h;
   lc = itofix(c1);
   rc = itofix(c2);

   /* check clipping */
   if (bmp->clip) {
      if (y1 < bmp->ct) {
	 lc += mc1 * (bmp->ct - y1);
	 rc += mc2 * (bmp->ct - y1);
	 y1 = bmp->ct;
      }
      y2 = MIN(y2, bmp->cb);
      x1 = MAX(x1, bmp->cl);
      x2 = MIN(x2, bmp->cr);
   }

   for (j=y1; j<y2; j++) {
      /* set up horizontal gradient for line */
      mh = (rc - lc) / sprite->w;
      hc = lc;

      /* more clip checking */
      if ((bmp->clip) && (x < bmp->cl))
	 hc += mh * (bmp->cl - x);

   #ifdef ALLEGRO_GFX_HAS_VGA

      /* modex version */
      if (is_planar_bitmap(bmp)) {
	 addr = ((unsigned long)bmp->line[j]<<2) + x1;
	 for (i=x1; i<x2; i++) {
	    if (sprite->line[j-y][i-x]) {
	       outportw(0x3C4, (0x100<<(i&3))|2);
	       pixel = color_map->data[fixtoi(hc)][sprite->line[j-y][i-x]];
	       bmp_write8(addr>>2, pixel);
	    }
	    hc += mh;
	    addr++;
	 }
      }
      else {

   #else

      {

   #endif

	 /* draw routines for all linear modes */
	 switch (bitmap_color_depth(bmp)) {

	 #ifdef ALLEGRO_COLOR8

	    case 8:
	       addr = bmp_write_line(bmp, j) + x1;
	       for (i=x1; i<x2; i++) {
		  if (sprite->line[j-y][i-x]) {
		     pixel = color_map->data[fixtoi(hc)][sprite->line[j-y][i-x]];
		     bmp_write8(addr, pixel);
		  }
		  hc += mh;
		  addr++;
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       addr = bmp_write_line(bmp, j) + x1*sizeof(short);
	       for (i=x1; i<x2; i++) {
		  pixel = ((unsigned short *)sprite->line[j-y])[i-x];
		  if (pixel != bmp->vtable->mask_color) {
		     if (bitmap_color_depth(bmp) == 16)
			pixel = _blender_func16(pixel, _blender_col_16, fixtoi(hc));
		     else
			pixel = _blender_func15(pixel, _blender_col_15, fixtoi(hc));
		     bmp_write16(addr, pixel);
		  }
		  hc += mh;
		  addr += sizeof(short);
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       addr = bmp_write_line(bmp, j) + x1*3;
	       for (i=x1; i<x2; i++) {
		  bmp_select(sprite);
		  pixel = bmp_read24((unsigned long)(sprite->line[j-y] + (i-x)*3));
		  bmp_select(bmp);
		  if (pixel != MASK_COLOR_24) {
		     pixel = _blender_func24(pixel, _blender_col_24, fixtoi(hc));
		     bmp_write24(addr, pixel);
		  }
		  hc += mh;
		  addr += 3;
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       addr = bmp_write_line(bmp, j) + x1*sizeof(int32_t);
	       for (i=x1; i<x2; i++) {
		  pixel = ((unsigned long *)sprite->line[j-y])[i-x];
		  if (pixel != MASK_COLOR_32) {
		     pixel = _blender_func32(pixel, _blender_col_32, fixtoi(hc));
		     bmp_write32(addr, pixel);
		  }
		  hc += mh;
		  addr += sizeof(int32_t);
	       }
	       break;

	 #endif

	 }
      }

      lc += mc1;
      rc += mc2;
   }

   bmp_unwrite_line(bmp);
}
