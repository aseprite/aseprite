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
 *      QNX Photon acceleration.
 *
 *      By Eric Botcazou.
 *
 *      Based on src/win/wddaccel by Stephan Schimanski and al.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
   #error Something is wrong with the makefile
#endif


/* state variables */
static PgColor_t chroma_color;
static int chroma_on = FALSE;

/* software version pointers */
static void (*_orig_hline) (BITMAP * bmp, int x1, int y, int x2, int color);
static void (*_orig_vline) (BITMAP * bmp, int x, int y1, int y2, int color);
static void (*_orig_rectfill) (BITMAP * bmp, int x1, int y1, int x2, int y2, int color);
static void (*_orig_draw_sprite) (BITMAP * bmp, BITMAP * sprite, int x, int y);
static void (*_orig_masked_blit) (BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);



/* photon_blit_to_self:
 *  Accelerated vram -> vram blitting routine.
 */
static void photon_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   struct Ph_rect source_rect = {
      { source_x + source->x_ofs,
        source_y + source->y_ofs },
      { source_x + source->x_ofs + width - 1,
        source_y + source->y_ofs + height - 1 }
   };
   
   struct Ph_rect dest_rect = {
      { dest_x + dest->x_ofs,
        dest_y + dest->y_ofs },
      { dest_x + dest->x_ofs + width - 1,
        dest_y + dest->y_ofs + height - 1 }
   };

   struct BITMAP *source_parent, *dest_parent;

   /* find parents */
   source_parent = source;
   while (source_parent->id & BMP_ID_SUB)
      source_parent = (BITMAP *)source_parent->extra;
   
   dest_parent = dest;
   while (dest_parent->id & BMP_ID_SUB)
      dest_parent = (BITMAP *)dest_parent->extra;

   PgContextBlit(BMP_EXTRA(source_parent)->context, &source_rect,
                 BMP_EXTRA(dest_parent)->context, &dest_rect);
   
   /* only for windowed mode */
   if (dest_parent == pseudo_screen)
      ph_update_window(&dest_rect);
}



/* ddraw_masked_blit:
 *  Accelerated masked blitting routine.
 */
static void photon_masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   struct Ph_rect source_rect = {
      { source_x + source->x_ofs,
        source_y + source->y_ofs },
      { source_x + source->x_ofs + width - 1,
        source_y + source->y_ofs + height - 1 }
   };
   
   struct Ph_rect dest_rect = {
      { dest_x + dest->x_ofs,
        dest_y + dest->y_ofs },
      { dest_x + dest->x_ofs + width - 1,
        dest_y + dest->y_ofs + height - 1 }
   };
   
   struct BITMAP *dest_parent, *source_parent;

   if (is_video_bitmap(source)) {
      if (!chroma_on) {
         PgChromaOn();
         chroma_on = TRUE;
      }
      
      /* find parents */
      source_parent = source;
      while (source_parent->id & BMP_ID_SUB)
         source_parent = (BITMAP *)source_parent->extra;
         
      dest_parent = dest;
      while (dest_parent->id & BMP_ID_SUB)
         dest_parent = (BITMAP *)dest_parent->extra;

      PgContextBlit(BMP_EXTRA(source_parent)->context, &source_rect,
                    BMP_EXTRA(dest_parent)->context, &dest_rect);

      /* only for windowed mode */
      if (dest_parent == pseudo_screen)
         ph_update_window(&dest_rect);
   }
   else {
      /* have to use the original software version */
      _orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
   }
}



/* photon_draw_sprite:
 *  Accelerated sprite drawing routine.
 */
static void photon_draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int sx, sy, w, h;

   if (is_video_bitmap(sprite)) {
      sx = sprite->x_ofs;
      sy = sprite->y_ofs;
      w = sprite->w;
      h = sprite->h;

      if (bmp->clip) {
	 if (x < bmp->cl) {
	    sx += bmp->cl - x;
	    w -= bmp->cl - x;
	    x = bmp->cl;
	 }

	 if (y < bmp->ct) {
	    sy += bmp->ct - y;
	    h -= bmp->ct - y;
	    y = bmp->ct;
	 }

	 if (x + w > bmp->cr)
	    w = bmp->cr - x;

	 if (w <= 0)
	    return;

	 if (y + h > bmp->cb)
	    h = bmp->cb - y;

	 if (h <= 0)
	    return;
      }

      photon_masked_blit(sprite, bmp, sx, sy, x, y, w, h);
   }
   else {
      /* have to use the original software version */
      _orig_draw_sprite(bmp, sprite, x, y);
   }
}



/* photon_clear_to_color:
 *  Accelerated screen clear routine.
 */
static void photon_clear_to_color(BITMAP *bmp, int color)
{
   struct Ph_rect dest_rect = {
      { bmp->cl + bmp->x_ofs,
        bmp->ct + bmp->y_ofs },
      { bmp->x_ofs + bmp->cr,
        bmp->y_ofs + bmp->cb }
   };

   struct BITMAP *parent;

   /* find parent */
   parent = bmp;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   /* if (bmp->vtable->color_depth == 8)
      PgSetFillColor(color);
   else */
      PgSetFillColor(PgRGB(getr(color), getg(color), getb(color)));
      
   PhDCSetCurrent(BMP_EXTRA(parent)->context);
   PgDrawRect(&dest_rect, Pg_DRAW_FILL);

   if (parent == pseudo_screen)
      ph_update_window(&dest_rect);
   else
      PgFlush();
}



/* photon_rectfill:
 *  Accelerated rectangle fill routine.
 */
static void photon_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   struct Ph_rect dest_rect;
   struct BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_rectfill(bmp, x1, y1, x2, y2, color);
      return;
   }

   if (x2 < x1) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (y2 < y1) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
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
   }

   dest_rect.ul.x = x1 + bmp->x_ofs;
   dest_rect.ul.y = y1 + bmp->y_ofs;
   dest_rect.lr.x = x2 + bmp->x_ofs;
   dest_rect.lr.y = y2 + bmp->y_ofs;

   /* find parent */
   parent = bmp;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   /* if (bmp->vtable->color_depth == 8)
      PgSetFillColor(color);
   else */
      PgSetFillColor(PgRGB(getr(color), getg(color), getb(color)));
      
   PhDCSetCurrent(BMP_EXTRA(parent)->context);
   PgDrawRect(&dest_rect, Pg_DRAW_FILL);

   if (parent == pseudo_screen)
      ph_update_window(&dest_rect);
   else
      PgFlush();
}



/* ddraw_hline:
 *  Accelerated scanline fill routine.
 */
static void ddraw_hline(BITMAP *bmp, int x1, int y, int x2, int color)
{
   struct Ph_rect dest_rect;
   struct BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_hline(bmp, x1, y, x2, color);
      return;
   }

   if (x1 > x2) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (bmp->clip) {
      if ((y < bmp->ct) || (y >= bmp->cb))
	 return;

      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;
   }
   
   dest_rect.ul.x = x1 + bmp->x_ofs;
   dest_rect.ul.y = y + bmp->y_ofs;
   dest_rect.lr.x = x2 + bmp->x_ofs;
   dest_rect.lr.y = y + bmp->y_ofs;
   
   /* find parent */
   parent = bmp;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   PhDCSetCurrent(BMP_EXTRA(parent)->context);
   PgDrawLine(&dest_rect.ul, &dest_rect.lr);

   if (parent == pseudo_screen)
      ph_update_window(&dest_rect);
   else
      PgFlush();
}


/* ddraw_vline:
 *  Accelerated vline routine.
 */
static void ddraw_vline(BITMAP *bmp, int x, int y1, int y2, int color)
{
   struct Ph_rect dest_rect;
   struct BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_vline(bmp, x, y1, y2, color);
      return;
   }

   if (y1 > y2) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if ((x < bmp->cl) || (x >= bmp->cr))
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;
   }

   dest_rect.ul.x = x + bmp->x_ofs;
   dest_rect.ul.y = y1 + bmp->y_ofs;
   dest_rect.lr.x = x + bmp->x_ofs;
   dest_rect.lr.y = y2 + bmp->y_ofs;

   /* find parent */
   parent = bmp;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   PhDCSetCurrent(BMP_EXTRA(parent)->context);
   PgDrawLine(&dest_rect.ul, &dest_rect.lr);

   if (parent == pseudo_screen)
      ph_update_window(&dest_rect);
   else
      PgFlush();
}



/* enable_acceleration:
 *  Checks graphic driver for capabilities to accelerate Allegro.
 */
void enable_acceleration(GFX_DRIVER * drv)
{
   /* safe pointer to software versions */
   _orig_hline = _screen_vtable.hline;
   _orig_vline = _screen_vtable.vline;
   _orig_rectfill = _screen_vtable.rectfill;
   _orig_draw_sprite = _screen_vtable.draw_sprite;
   _orig_masked_blit = _screen_vtable.masked_blit;

   /* accelerated video to video blits? */
   if (ph_gfx_mode_info.mode_capabilities2 & PgVM_MODE_CAP2_BITBLT) {
      _screen_vtable.blit_to_self = photon_blit_to_self;
      
      /* overlapping blits seem not to be supported */
   #if 0
      _screen_vtable.blit_to_self_forward = photon_blit_to_self;
      _screen_vtable.blit_to_self_backward = photon_blit_to_self;
   #endif

      gfx_capabilities |= GFX_HW_VRAM_BLIT;
   }

   /* accelerated color fills? */
   if (ph_gfx_mode_info.mode_capabilities2 & PgVM_MODE_CAP2_RECTANGLE) {
      _screen_vtable.clear_to_color = photon_clear_to_color;
      _screen_vtable.rectfill = photon_rectfill;

      gfx_capabilities |= GFX_HW_FILL;
   }
   
   /* accelerated lines? */
   if (ph_gfx_mode_info.mode_capabilities2 & PgVM_MODE_CAP2_LINES) {
      _screen_vtable.hline = ddraw_hline;
      _screen_vtable.vline = ddraw_vline;

      gfx_capabilities |= GFX_HW_HLINE;
   }
   
   /* accelerated color key blits? */
   if (ph_gfx_mode_info.mode_capabilities2 & PgVM_MODE_CAP2_CHROMA) {
      _screen_vtable.masked_blit = photon_masked_blit;
      _screen_vtable.draw_sprite = photon_draw_sprite;

      if (_screen_vtable.color_depth == 8)
	 _screen_vtable.draw_256_sprite = photon_draw_sprite;

      gfx_capabilities |= GFX_HW_VRAM_BLIT_MASKED;
   }
}



/* enable_triple_buffering:
 *  Checks graphic driver for triple buffering capability.
 */
void enable_triple_buffering(GFX_DRIVER *drv)
{
   if (ph_gfx_mode_info.mode_capabilities1 & PgVM_MODE_CAP1_TRIPLE_BUFFER) {
      /* drv->poll_scroll = qnx_ph_poll_scroll; */
      gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }
}
