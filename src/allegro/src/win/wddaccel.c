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
 *      DirectDraw acceleration.
 *
 *      By Stefan Schimanski.
 *
 *      Bugfixes by Isaac Cruz.
 *
 *      Accelerated rectfill() and hline() added by Shawn Hargreaves.
 *
 *      Accelerated stretch_blit() and friends by Thomas Harte.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"



#define PREFIX_I                "al-ddaccel INFO: "
#define PREFIX_W                "al-ddaccel WARNING: "
#define PREFIX_E                "al-ddaccel ERROR: "

/* software version pointers */
static void (*_orig_hline) (BITMAP * bmp, int x1, int y, int x2, int color);
static void (*_orig_vline) (BITMAP * bmp, int x, int y1, int y2, int color);
static void (*_orig_rectfill) (BITMAP * bmp, int x1, int y1, int x2, int y2, int color);
static void (*_orig_draw_sprite) (BITMAP * bmp, BITMAP * sprite, int x, int y);
static void (*_orig_masked_blit) (BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void (*_orig_stretch_blit) (BITMAP *source, BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked);



/* ddraw_blit_to_self:
 *  Accelerated vram -> vram blitting routine.
 */
static void ddraw_blit_to_self(BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   RECT src_rect;
   int dest_parent_x = dest_x + dest->x_ofs;
   int dest_parent_y = dest_y + dest->y_ofs;
   BITMAP *dest_parent;
   BITMAP *source_parent;

   src_rect.left = source_x + source->x_ofs;
   src_rect.top = source_y + source->y_ofs;
   src_rect.right = source_x + source->x_ofs + width;
   src_rect.bottom = source_y + source->y_ofs + height;

   /* find parents */
   dest_parent = dest;
   while (dest_parent->id & BMP_ID_SUB)
      dest_parent = (BITMAP *)dest_parent->extra;

   source_parent = source;
   while (source_parent->id & BMP_ID_SUB)
      source_parent = (BITMAP *)source_parent->extra;
   
   _enter_gfx_critical();
   gfx_directx_release_lock(dest);
   gfx_directx_release_lock(source);
   
   IDirectDrawSurface2_BltFast(DDRAW_SURFACE_OF(dest_parent)->id, dest_parent_x, dest_parent_y,
                               DDRAW_SURFACE_OF(source_parent)->id, &src_rect,
                               DDBLTFAST_WAIT);
   _exit_gfx_critical();
   
   /* only for windowed mode */
   if ((gfx_driver->id == GFX_DIRECTX_WIN) && (dest_parent == gfx_directx_forefront_bitmap)) {
      src_rect.left   = dest_parent_x;
      src_rect.top    = dest_parent_y;
      src_rect.right  = dest_parent_x + width;
      src_rect.bottom = dest_parent_y + height;
      win_gfx_driver->paint(&src_rect);
   }
}


/* ddraw_masked_blit:
 *  Accelerated masked blitting routine.
 */
static void ddraw_masked_blit(BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   RECT dest_rect, source_rect;
   DDCOLORKEY src_key;
   HRESULT hr;
   BITMAP *dest_parent;
   BITMAP *source_parent;

   dest_rect.left = dest_x + dest->x_ofs;
   dest_rect.top = dest_y + dest->y_ofs;
   dest_rect.right = dest_x + dest->x_ofs + width;
   dest_rect.bottom = dest_y + dest->y_ofs + height;

   source_rect.left = source_x + source->x_ofs;
   source_rect.top = source_y + source->y_ofs;
   source_rect.right = source_x + source->x_ofs + width;
   source_rect.bottom = source_y + source->y_ofs + height;

   src_key.dwColorSpaceLowValue = source->vtable->mask_color;
   src_key.dwColorSpaceHighValue = source->vtable->mask_color;

   if (is_video_bitmap(source) || is_system_bitmap(source)) {

      /* find parents */
      dest_parent = dest;
      while (dest_parent->id & BMP_ID_SUB)
         dest_parent = (BITMAP *)dest_parent->extra;

      source_parent = source;
      while (source_parent->id & BMP_ID_SUB)
         source_parent = (BITMAP *)source_parent->extra;

      _enter_gfx_critical();
      gfx_directx_release_lock(dest);
      gfx_directx_release_lock(source);

      IDirectDrawSurface2_SetColorKey(DDRAW_SURFACE_OF(source_parent)->id,
                                      DDCKEY_SRCBLT, &src_key);

      hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(dest_parent)->id, &dest_rect,
                                   DDRAW_SURFACE_OF(source_parent)->id, &source_rect,
                                   DDBLT_KEYSRC | DDBLT_WAIT, NULL);
      _exit_gfx_critical();

      if (FAILED(hr))
	 _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

      /* only for windowed mode */
      if ((gfx_driver->id == GFX_DIRECTX_WIN) && (dest_parent == gfx_directx_forefront_bitmap))
         win_gfx_driver->paint(&dest_rect);
   }
   else {
      /* have to use the original software version */
      _orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
   }
}



/* ddraw_draw_sprite:
 *  Accelerated sprite drawing routine.
 */
static void ddraw_draw_sprite(BITMAP * bmp, BITMAP * sprite, int x, int y)
{
   int sx, sy, w, h;

   if (is_video_bitmap(sprite) || is_system_bitmap(sprite)) {
      sx = 0;  /* sprite->x_ofs & sprite->y_ofs will be accounted for in ddraw_masked_blit */
      sy = 0;
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

      ddraw_masked_blit(sprite, bmp, sx, sy, x, y, w, h);
   }
   else {
      /* have to use the original software version */
      _orig_draw_sprite(bmp, sprite, x, y);
   }
}



/* ddraw_do_stretch_blit:
 *   Accelerated stretch_blit, stretch_sprite, stretch_masked_blit
 */
static void ddraw_do_stretch_blit(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked)
{
   RECT dest_rect, source_rect;
   DDCOLORKEY src_key;
   HRESULT hr;
   BITMAP *dest_parent;
   BITMAP *source_parent;

   dest_rect.left = dest_x + dest->x_ofs;
   dest_rect.top = dest_y + dest->y_ofs;
   dest_rect.right = dest_x + dest->x_ofs + dest_width;
   dest_rect.bottom = dest_y + dest->y_ofs + dest_height;

   source_rect.left = source_x + source->x_ofs;
   source_rect.top = source_y + source->y_ofs;
   source_rect.right = source_x + source->x_ofs + source_width;
   source_rect.bottom = source_y + source->y_ofs + source_height;

   src_key.dwColorSpaceLowValue = source->vtable->mask_color;
   src_key.dwColorSpaceHighValue = source->vtable->mask_color;

   if ( ( (masked && (gfx_capabilities & GFX_HW_VRAM_STRETCH_BLIT_MASKED)) ||
          (!masked && (gfx_capabilities & GFX_HW_VRAM_STRETCH_BLIT)) 
        ) && ( is_video_bitmap(source) || is_system_bitmap(source) ) ) {

      /* find parents */
      dest_parent = dest;
      while (dest_parent->id & BMP_ID_SUB)
         dest_parent = (BITMAP *)dest_parent->extra;

      source_parent = source;
      while (source_parent->id & BMP_ID_SUB)
         source_parent = (BITMAP *)source_parent->extra;

      _enter_gfx_critical();
      gfx_directx_release_lock(dest);
      gfx_directx_release_lock(source);

      IDirectDrawSurface2_SetColorKey(DDRAW_SURFACE_OF(source_parent)->id,
                                      DDCKEY_SRCBLT, &src_key);

      hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(dest_parent)->id, &dest_rect,
                                   DDRAW_SURFACE_OF(source_parent)->id, &source_rect,
                                   (masked ? DDBLT_KEYSRC : 0) | DDBLT_WAIT, NULL);
      _exit_gfx_critical();

      if (FAILED(hr))
	 _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

      /* only for windowed mode */
      if ((gfx_driver->id == GFX_DIRECTX_WIN) && (dest_parent == gfx_directx_forefront_bitmap))
         win_gfx_driver->paint(&dest_rect);
   }
   else {
      /* have to use the original software version */
      _orig_stretch_blit(source, dest, source_x, source_y, source_width, source_height, dest_x, dest_y, dest_width, dest_height, masked);
   }
}



/* ddraw_clear_to_color:
 *  Accelerated screen clear routine.
 */
static void ddraw_clear_to_color(BITMAP * bitmap, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   dest_rect.left = bitmap->cl + bitmap->x_ofs;
   dest_rect.top = bitmap->ct + bitmap->y_ofs;
   dest_rect.right = bitmap->x_ofs + bitmap->cr;
   dest_rect.bottom = bitmap->y_ofs + bitmap->cb;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(parent)->id, &dest_rect,
                                NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);
   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

   /* only for windowed mode */
   if ((gfx_driver->id == GFX_DIRECTX_WIN) && (parent == gfx_directx_forefront_bitmap))
      win_gfx_driver->paint(&dest_rect);
}



/* ddraw_rectfill:
 *  Accelerated rectangle fill routine.
 */
static void ddraw_rectfill(BITMAP *bitmap, int x1, int y1, int x2, int y2, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_rectfill(bitmap, x1, y1, x2, y2, color);
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

   if (bitmap->clip) {
      if (x1 < bitmap->cl)
	 x1 = bitmap->cl;

      if (x2 >= bitmap->cr)
	 x2 = bitmap->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bitmap->ct)
	 y1 = bitmap->ct;

      if (y2 >= bitmap->cb)
	 y2 = bitmap->cb-1;

      if (y2 < y1)
	 return;
   }

   dest_rect.left = x1 + bitmap->x_ofs;
   dest_rect.top = y1 + bitmap->y_ofs;
   dest_rect.right = x2 + bitmap->x_ofs + 1;
   dest_rect.bottom = y2 + bitmap->y_ofs + 1;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(parent)->id, &dest_rect,
                                NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);
   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

   /* only for windowed mode */
   if ((gfx_driver->id == GFX_DIRECTX_WIN) && (parent == gfx_directx_forefront_bitmap))
      win_gfx_driver->paint(&dest_rect);
}



/* ddraw_hline:
 *  Accelerated scanline fill routine.
 */
static void ddraw_hline(BITMAP *bitmap, int x1, int y, int x2, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_hline(bitmap, x1, y, x2, color);
      return;
   }

   if (x1 > x2) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (bitmap->clip) {
      if ((y < bitmap->ct) || (y >= bitmap->cb))
	 return;

      if (x1 < bitmap->cl)
	 x1 = bitmap->cl;

      if (x2 >= bitmap->cr)
	 x2 = bitmap->cr-1;

      if (x2 < x1)
	 return;
   }

   dest_rect.left = x1 + bitmap->x_ofs;
   dest_rect.top = y + bitmap->y_ofs;
   dest_rect.right = x2 + bitmap->x_ofs + 1;
   dest_rect.bottom = y + bitmap->y_ofs + 1;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(parent)->id, &dest_rect,
                                NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);
   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

   /* only for windowed mode */
   if ((gfx_driver->id == GFX_DIRECTX_WIN) && (parent == gfx_directx_forefront_bitmap))
      win_gfx_driver->paint(&dest_rect);
}


/* ddraw_vline:
 *  Accelerated vline routine.
 */
static void ddraw_vline(BITMAP *bitmap, int x, int y1, int y2, int color)
{
   RECT dest_rect;
   HRESULT hr;
   DDBLTFX blt_fx;
   BITMAP *parent;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_vline(bitmap, x, y1, y2, color);
      return;
   }

   if (y1 > y2) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bitmap->clip) {
      if ((x < bitmap->cl) || (x >= bitmap->cr))
	 return;

      if (y1 < bitmap->ct)
	 y1 = bitmap->ct;

      if (y2 >= bitmap->cb)
	 y2 = bitmap->cb-1;

      if (y2 < y1)
	 return;
   }

   dest_rect.top = y1 + bitmap->y_ofs;
   dest_rect.left = x + bitmap->x_ofs;
   dest_rect.bottom = y2 + bitmap->y_ofs + 1;
   dest_rect.right = x + bitmap->x_ofs + 1;

   /* find parent */
   parent = bitmap;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *)parent->extra;

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_release_lock(bitmap);

   hr = IDirectDrawSurface2_Blt(DDRAW_SURFACE_OF(parent)->id, &dest_rect,
                                NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);
   _exit_gfx_critical();

   if (FAILED(hr))
      _TRACE(PREFIX_E "Blt failed (%x)\n", hr);

   /* only for windowed mode */
   if ((gfx_driver->id == GFX_DIRECTX_WIN) && (parent == gfx_directx_forefront_bitmap))
      win_gfx_driver->paint(&dest_rect);
}



/* gfx_directx_enable_acceleration:
 *  Checks graphic driver for capabilities to accelerate Allegro.
 */
void gfx_directx_enable_acceleration(GFX_DRIVER * drv)
{
   /* safe pointer to software versions */
   _orig_hline = _screen_vtable.hline;
   _orig_vline = _screen_vtable.vline;
   _orig_rectfill = _screen_vtable.rectfill;
   _orig_draw_sprite = _screen_vtable.draw_sprite;
   _orig_masked_blit = _screen_vtable.masked_blit;
   _orig_stretch_blit = _screen_vtable.do_stretch_blit;

   /* accelerated video to video blits? */
   if (ddcaps.dwCaps & DDCAPS_BLT) {
      _screen_vtable.blit_to_self = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_forward = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_backward = ddraw_blit_to_self;
      _screen_vtable.blit_from_system = ddraw_blit_to_self;
      _screen_vtable.blit_to_system = ddraw_blit_to_self;

      if (ddcaps.dwCaps & DDCAPS_BLTSTRETCH) {
         _screen_vtable.do_stretch_blit = ddraw_do_stretch_blit;
         gfx_capabilities |= GFX_HW_VRAM_STRETCH_BLIT | GFX_HW_SYS_STRETCH_BLIT;
      }

      gfx_capabilities |= (GFX_HW_VRAM_BLIT | GFX_HW_SYS_TO_VRAM_BLIT);
   }

   /* accelerated color fills? */
   if (ddcaps.dwCaps & DDCAPS_BLTCOLORFILL) {
      _screen_vtable.clear_to_color = ddraw_clear_to_color;
      _screen_vtable.rectfill = ddraw_rectfill;
      _screen_vtable.hline = ddraw_hline;
      _screen_vtable.vline = ddraw_vline;

      gfx_capabilities |= (GFX_HW_HLINE | GFX_HW_FILL);
   }

   /* accelerated color key blits? */
   if ((ddcaps.dwCaps & DDCAPS_COLORKEY) &&
       (ddcaps.dwCKeyCaps & DDCKEYCAPS_SRCBLT)) {
      _screen_vtable.masked_blit = ddraw_masked_blit;
      _screen_vtable.draw_sprite = ddraw_draw_sprite;

      if (ddcaps.dwCaps & DDCAPS_BLTSTRETCH) {
         _screen_vtable.do_stretch_blit = ddraw_do_stretch_blit;
         gfx_capabilities |= 
            GFX_HW_VRAM_STRETCH_BLIT_MASKED|GFX_HW_SYS_STRETCH_BLIT_MASKED;
      }

      if (_screen_vtable.color_depth == 8)
	 _screen_vtable.draw_256_sprite = ddraw_draw_sprite;

      gfx_capabilities |= (GFX_HW_VRAM_BLIT_MASKED | GFX_HW_SYS_TO_VRAM_BLIT_MASKED);
   }
}



/* gfx_directx_enable_triple_buffering:
 *  Checks graphic driver for triple buffering capability.
 */
void gfx_directx_enable_triple_buffering(GFX_DRIVER *drv)
{
   HRESULT hr;

   hr = IDirectDrawSurface2_GetFlipStatus(DDRAW_SURFACE_OF(gfx_directx_forefront_bitmap)->id, DDGFS_ISFLIPDONE);
   if ((hr == DD_OK) || (hr == DDERR_WASSTILLDRAWING)) {
      drv->poll_scroll = gfx_directx_poll_scroll;
      gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }
}

