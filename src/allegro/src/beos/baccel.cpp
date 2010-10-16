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
 *      Accelerated gfx routines for BeOS.
 *
 *      Based on similar functions for Windows/X, by Jason Wilkins
 *      and Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif



/* our hooks table */
HOOKS _be_hooks;

/* the sync function pointer */
int32 (*_be_sync_func)() = NULL;



/* software version pointers */
static void (*_orig_hline) (BITMAP *bmp, int x1, int y, int x2, int color);
static void (*_orig_vline) (BITMAP *bmp, int x, int y1, int y2, int color);
static void (*_orig_rectfill) (BITMAP *bmp, int x1, int y1, int x2, int y2, int color);


#define MAKE_HLINE(bpp)														\
static void be_gfx_accel_hline_##bpp(BITMAP *bmp, int x1, int y, int x2, int color)	\
{																			\
   if (_drawing_mode != DRAW_MODE_SOLID) {									\
      _orig_hline(bmp, x1, y, x2, color);									\
      return;																\
   }																		\
   																			\
   if (x1 > x2) {															\
      int tmp = x1;															\
      x1 = x2;																\
      x2 = tmp;																\
   }																		\
																			\
   if (bmp->clip) {															\
      if ((y < bmp->ct) || (y >= bmp->cb))									\
	 return;																\
																			\
      if (x1 < bmp->cl)														\
	 x1 = bmp->cl;															\
																			\
      if (x2 >= bmp->cr)													\
	 x2 = bmp->cr-1;														\
																			\
      if (x2 < x1)															\
	 return;																\
   }																		\
																			\
   x1 += bmp->x_ofs;														\
   y += bmp->y_ofs;															\
   x2 += bmp->x_ofs;														\
																			\
   if (_be_lock_count)														\
      _be_hooks.draw_rect_##bpp(x1, y, x2, y, color);						\
   else {																	\
      acquire_sem(_be_fullscreen_lock);										\
      _be_hooks.draw_rect_##bpp(x1, y, x2, y, color);						\
      release_sem(_be_fullscreen_lock);										\
   }																		\
   bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);							\
}



#define MAKE_VLINE(bpp)														\
static void be_gfx_accel_vline_##bpp(BITMAP *bmp, int x, int y1, int y2, int color) \
{																			\
   if (_drawing_mode != DRAW_MODE_SOLID) {									\
      _orig_vline(bmp, x, y1, y2, color);									\
      return;																\
   }																		\
																			\
   if (y1 > y2) {															\
      int tmp = y1;															\
      y1 = y2;																\
      y2 = tmp;																\
   }																		\
																			\
   if (bmp->clip) {															\
      if ((x < bmp->cl) || (x >= bmp->cr))									\
	 return;																\
																			\
      if (y1 < bmp->ct)														\
	 y1 = bmp->ct;															\
																			\
      if (y2 >= bmp->cb)													\
	 y2 = bmp->cb-1;														\
																			\
      if (y2 < y1)															\
	 return;																\
   }																		\
   																			\
   x += bmp->x_ofs;															\
   y1 += bmp->y_ofs;														\
   y2 += bmp->y_ofs;														\
																			\
   if (_be_lock_count)														\
      _be_hooks.draw_rect_##bpp(x, y1, x, y2, color);						\
   else {																	\
      acquire_sem(_be_fullscreen_lock);										\
      _be_hooks.draw_rect_##bpp(x, y1, x, y2, color);						\
      release_sem(_be_fullscreen_lock);										\
   }																		\
   bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);							\
}


#define MAKE_RECTFILL(bpp)													\
static void be_gfx_accel_rectfill_##bpp(BITMAP *bmp, int x1, int y1, int x2, int y2, int color) \
{																			\
   if (_drawing_mode != DRAW_MODE_SOLID) {									\
      _orig_rectfill(bmp, x1, y1, x2, y2, color);							\
      return;																\
   }																		\
																			\
   if (x2 < x1) {															\
      int tmp = x1;															\
      x1 = x2;																\
      x2 = tmp;																\
   }																		\
																			\
   if (y2 < y1) {															\
      int tmp = y1;															\
      y1 = y2;																\
      y2 = tmp;																\
   }																		\
																			\
   if (bmp->clip) {															\
      if (x1 < bmp->cl)														\
	 x1 = bmp->cl;															\
																			\
      if (x2 >= bmp->cr)													\
	 x2 = bmp->cr-1;														\
																			\
      if (x2 < x1)															\
	 return;																\
																			\
      if (y1 < bmp->ct)														\
	 y1 = bmp->ct;															\
																			\
      if (y2 >= bmp->cb)													\
	 y2 = bmp->cb-1;														\
																			\
      if (y2 < y1)															\
	 return;																\
   }																		\
   																			\
   x1 += bmp->x_ofs;														\
   y1 += bmp->y_ofs;														\
   x2 += bmp->x_ofs;														\
   y2 += bmp->y_ofs;														\
																			\
   if (_be_lock_count)														\
      _be_hooks.draw_rect_##bpp(x1, y1, x2, y2, color);						\
   else {																	\
      acquire_sem(_be_fullscreen_lock);										\
      _be_hooks.draw_rect_##bpp(x1, y1, x2, y2, color);						\
      release_sem(_be_fullscreen_lock);										\
   }																		\
   bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);							\
}


#define MAKE_CLEAR_TO_COLOR(bpp)											\
static void be_gfx_accel_clear_to_color_##bpp(BITMAP *bmp, int color)		\
{																			\
   int x1, y1, x2, y2;														\
																			\
   x1 = bmp->cl + bmp->x_ofs;												\
   y1 = bmp->ct + bmp->y_ofs;												\
   x2 = bmp->cr + bmp->x_ofs - 1;											\
   y2 = bmp->cb + bmp->y_ofs - 1;											\
																			\
   if (_be_lock_count)														\
      _be_hooks.draw_rect_##bpp(x1, y1, x2, y2, color);						\
   else {																	\
      acquire_sem(_be_fullscreen_lock);										\
      _be_hooks.draw_rect_##bpp(x1, y1, x2, y2, color);						\
      release_sem(_be_fullscreen_lock);										\
   }																		\
   bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);							\
}


MAKE_HLINE(8);
MAKE_HLINE(16);
MAKE_HLINE(32);

MAKE_VLINE(8);
MAKE_VLINE(16);
MAKE_VLINE(32);

MAKE_RECTFILL(8);
MAKE_RECTFILL(16);
MAKE_RECTFILL(32);

MAKE_CLEAR_TO_COLOR(8);
MAKE_CLEAR_TO_COLOR(16);
MAKE_CLEAR_TO_COLOR(32);



/* be_gfx_accel_blit_to_self:
 *
 */
static void be_gfx_accel_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   source_x += source->x_ofs;
   source_y += source->y_ofs;
   dest_x += dest->x_ofs;
   dest_y += dest->y_ofs;

   if (_be_lock_count)
      _be_hooks.blit(source_x, source_y, dest_x, dest_y, width - 1, height - 1);
   else {
      acquire_sem(_be_fullscreen_lock);
      _be_hooks.blit(source_x, source_y, dest_x, dest_y, width - 1, height - 1);
      release_sem(_be_fullscreen_lock);
   }
   dest->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
}



/* be_gfx_bwindowscreen_accelerate:
 *  Detects and initializes hardware accelerated gfx functions.
 */
extern "C" void be_gfx_bwindowscreen_accelerate(int depth)
{
   _be_hooks.draw_line_8   = (LINE8_HOOK)  _be_allegro_screen->CardHookAt(LINE8_HOOK_NUM);
   _be_hooks.draw_line_16  = (LINE16_HOOK) _be_allegro_screen->CardHookAt(LINE16_HOOK_NUM);
   _be_hooks.draw_line_32  = (LINE32_HOOK) _be_allegro_screen->CardHookAt(LINE32_HOOK_NUM);
   _be_hooks.draw_array_8  = (ARRAY8_HOOK) _be_allegro_screen->CardHookAt(ARRAY8_HOOK_NUM);
   _be_hooks.draw_array_32 = (ARRAY32_HOOK)_be_allegro_screen->CardHookAt(ARRAY32_HOOK_NUM);
   _be_hooks.draw_rect_8   = (RECT8_HOOK)  _be_allegro_screen->CardHookAt(RECT8_HOOK_NUM);
   _be_hooks.draw_rect_16  = (RECT16_HOOK) _be_allegro_screen->CardHookAt(RECT16_HOOK_NUM);
   _be_hooks.draw_rect_32  = (RECT32_HOOK) _be_allegro_screen->CardHookAt(RECT32_HOOK_NUM);
   _be_hooks.invert_rect   = (INVERT_HOOK) _be_allegro_screen->CardHookAt(INVERT_HOOK_NUM);
   _be_hooks.blit          = (BLIT_HOOK)   _be_allegro_screen->CardHookAt(BLIT_HOOK_NUM);
   _be_hooks.sync          = (SYNC_HOOK)   _be_allegro_screen->CardHookAt(SYNC_HOOK_NUM);

   gfx_capabilities = 0;

   if (!_be_hooks.sync)
      return;
   
   _orig_hline = _screen_vtable.hline;
   _orig_vline = _screen_vtable.vline;
   _orig_rectfill = _screen_vtable.rectfill;
   
   switch (depth) {
      case 8:
         if ((_be_hooks.draw_line_8) || (_be_hooks.draw_rect_8)) {
            _screen_vtable.hline = be_gfx_accel_hline_8;
            _screen_vtable.vline = be_gfx_accel_vline_8;
         }
         if (_be_hooks.draw_line_8) {
            gfx_capabilities |= GFX_HW_LINE;
         }
         if (_be_hooks.draw_rect_8) {
            _screen_vtable.rectfill = be_gfx_accel_rectfill_8;
            _screen_vtable.clear_to_color = be_gfx_accel_clear_to_color_8;
            gfx_capabilities |= GFX_HW_FILL;
         }
         break;

      case 15:
      case 16:
         if ((_be_hooks.draw_line_16) || (_be_hooks.draw_rect_16)) {
            _screen_vtable.hline = be_gfx_accel_hline_16;
            _screen_vtable.vline = be_gfx_accel_vline_16;
         }
         if (_be_hooks.draw_line_16) {
            gfx_capabilities |= GFX_HW_LINE;
         }
         if (_be_hooks.draw_rect_16) {
            _screen_vtable.rectfill = be_gfx_accel_rectfill_16;
            _screen_vtable.clear_to_color = be_gfx_accel_clear_to_color_16;
            gfx_capabilities |= GFX_HW_FILL;
         }
         break;

      case 32:
         if ((_be_hooks.draw_line_32) || (_be_hooks.draw_rect_32)) {
            _screen_vtable.hline = be_gfx_accel_hline_32;
            _screen_vtable.vline = be_gfx_accel_vline_32;
         }
         if (_be_hooks.draw_line_32) {
            gfx_capabilities |= GFX_HW_LINE;
         }
         if (_be_hooks.draw_rect_32) {
            _screen_vtable.rectfill = be_gfx_accel_rectfill_32;
            _screen_vtable.clear_to_color = be_gfx_accel_clear_to_color_32;
            gfx_capabilities |= GFX_HW_FILL;
         }
         break;
   }
   if (_be_hooks.invert_rect)
      gfx_capabilities |= GFX_HW_FILL_XOR;
   if (_be_hooks.blit) {
      _screen_vtable.blit_to_self = be_gfx_accel_blit_to_self;
      _screen_vtable.blit_to_self_forward = be_gfx_accel_blit_to_self;
      _screen_vtable.blit_to_self_backward = be_gfx_accel_blit_to_self;
      gfx_capabilities |= GFX_HW_VRAM_BLIT;
   }
   _be_sync_func = _be_hooks.sync;
}

