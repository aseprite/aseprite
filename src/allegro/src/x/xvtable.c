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
 *      Magical wrappers for functions in vtable.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

#ifdef ALLEGRO_MULTITHREADED
#include <pthread.h>
#endif


static GFX_VTABLE _xwin_vtable;


static void _xwin_putpixel(BITMAP *dst, int dx, int dy, int color);
static void _xwin_vline(BITMAP *dst, int dx, int dy1, int dy2, int color);
static void _xwin_hline(BITMAP *dst, int dx1, int dy, int dx2, int color);
static void _xwin_rectfill(BITMAP *dst, int dx1, int dy1, int dx2, int dy2, int color);
static void _xwin_draw_sprite(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_sprite_ex(BITMAP *dst, BITMAP *src, int dx, int dy, int mode, int flip);
static void _xwin_draw_256_sprite(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_sprite_v_flip(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_sprite_h_flip(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_sprite_vh_flip(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_trans_sprite(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_trans_rgba_sprite(BITMAP *dst, BITMAP *src, int dx, int dy);
static void _xwin_draw_lit_sprite(BITMAP *dst, BITMAP *src, int dx, int dy, int color);
static void _xwin_draw_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy);
static void _xwin_draw_trans_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy);
static void _xwin_draw_trans_rgba_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy);
static void _xwin_draw_lit_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy, int color);
static void _xwin_draw_character(BITMAP *dst, BITMAP *src, int dx, int dy, int color, int bg);
static void _xwin_draw_glyph(BITMAP *dst, AL_CONST FONT_GLYPH *src, int dx, int dy, int color, int bg);
static void _xwin_blit_anywhere(BITMAP *src, BITMAP *dst, int sx, int sy,
				int dx, int dy, int w, int h);
static void _xwin_blit_backward(BITMAP *src, BITMAP *dst, int sx, int sy,
				int dx, int dy, int w, int h);
static void _xwin_masked_blit(BITMAP *src, BITMAP *dst, int sx, int sy,
			      int dx, int dy, int w, int h);
static void _xwin_clear_to_color(BITMAP *dst, int color);



/* _xwin_drawing_mode:
 *  Set the GC's drawing mode
 */
void _xwin_drawing_mode(void)
{
   /* Only SOLID can be handled directly by X11. */
   if(_xwin.matching_formats && _drawing_mode == DRAW_MODE_SOLID)
      _xwin.drawing_mode_ok = TRUE;
   else
      _xwin.drawing_mode_ok = FALSE;
}



/* Direct X11 version of the function. */
static inline int _xwin_direct_putpixel(BITMAP *dst, int dx, int dy, int color)
{
   if (!_xwin.drawing_mode_ok)
      return 0;

   dx += dst->x_ofs - _xwin.scroll_x;
   dy += dst->y_ofs - _xwin.scroll_y;

   if((dx >= _xwin.screen_width) || (dx < 0) ||
      (dy >= _xwin.screen_height) || (dy < 0))
      return 1;

   XLOCK();
   XSetForeground(_xwin.display, _xwin.gc, color);
   XDrawPoint(_xwin.display, _xwin.window, _xwin.gc, dx, dy);
   XUNLOCK();

   return 1;
}



/* Direct X11 version of the function. */
static inline int _xwin_direct_hline(BITMAP *dst, int dx1, int dy, int dx2, int color)
{
   if (!_xwin.drawing_mode_ok)
      return 0;

   dx1 += dst->x_ofs - _xwin.scroll_x;
   dx2 += dst->x_ofs - _xwin.scroll_x;
   dy += dst->y_ofs - _xwin.scroll_y;

   if (dx1 < 0)
      dx1 = 0;
   if (dx2 >= _xwin.screen_width)
      dx2 = _xwin.screen_width - 1;
   if ((dx1 > dx2) || (dy < 0) || (dy >= _xwin.screen_height))
      return 1;

   XLOCK();
   XSetForeground(_xwin.display, _xwin.gc, color);
   XDrawLine(_xwin.display, _xwin.window, _xwin.gc, dx1, dy, dx2, dy);
   XUNLOCK();

   return 1;
}



/* Direct X11 version of the function. */
static inline int _xwin_direct_vline(BITMAP *dst, int dx, int dy1, int dy2, int color)
{
   if (!_xwin.drawing_mode_ok)
      return 0;

   dx += dst->x_ofs - _xwin.scroll_x;
   dy1 += dst->y_ofs - _xwin.scroll_y;
   dy2 += dst->y_ofs - _xwin.scroll_y;

   if (dy1 < 0)
      dy1 = 0;
   if (dy2 >= _xwin.screen_height)
      dy2 = _xwin.screen_height - 1;
   if ((dy1 > dy2) || (dx < 0) || (dx >= _xwin.screen_width))
      return 1;

   XLOCK();
   XSetForeground(_xwin.display, _xwin.gc, color);
   XDrawLine(_xwin.display, _xwin.window, _xwin.gc, dx, dy1, dx, dy2);
   XUNLOCK();

   return 1;
}



/* Direct X11 version of the function. */
static inline int _xwin_direct_rectfill(BITMAP *dst, int dx1, int dy1, int dx2, int dy2, int color)
{
   if (!_xwin.drawing_mode_ok)
      return 0;

   dx1 += dst->x_ofs - _xwin.scroll_x;
   dx2 += dst->x_ofs - _xwin.scroll_x;
   dy1 += dst->y_ofs - _xwin.scroll_y;
   dy2 += dst->y_ofs - _xwin.scroll_y;

   if (dx1 < 0)
      dx1 = 0;
   if (dx2 >= _xwin.screen_width)
      dx2 = _xwin.screen_width - 1;
   if (dx1 > dx2)
      return 1;

   if (dy1 < 0)
      dy1 = 0;
   if (dy2 >= _xwin.screen_height)
      dy2 = _xwin.screen_height - 1;
   if (dy1 > dy2)
      return 1;

   XLOCK();
   XSetForeground(_xwin.display, _xwin.gc, color);
   XFillRectangle(_xwin.display, _xwin.window, _xwin.gc, dx1, dy1, dx2-dx1+1, dy2-dy1+1);
   XUNLOCK();

   return 1;
}



/* Direct X11 version of the function. */
static inline int _xwin_direct_clear_to_color(BITMAP *dst, int color)
{
   int dx1, dy1, dx2, dy2;
   if (!_xwin.drawing_mode_ok)
      return 0;

   dx1 = dst->cl + dst->x_ofs - _xwin.scroll_x;
   dx2 = dst->cr + dst->x_ofs - 1 - _xwin.scroll_x;
   dy1 = dst->ct + dst->y_ofs - _xwin.scroll_y;
   dy2 = dst->cb + dst->y_ofs - 1 - _xwin.scroll_y;

   if (dx1 < 0)
      dx1 = 0;
   if (dx2 >= _xwin.screen_width)
      dx2 = _xwin.screen_width - 1;
   if (dx1 > dx2)
      return 1;

   if (dy1 < 0)
      dy1 = 0;
   if (dy2 >= _xwin.screen_height)
      dy2 = _xwin.screen_height - 1;
   if (dy1 > dy2)
      return 1;

   XLOCK();
   XSetForeground(_xwin.display, _xwin.gc, color);
   XFillRectangle(_xwin.display, _xwin.window, _xwin.gc, dx1, dy1, dx2-dx1+1, dy2-dy1+1);
   XUNLOCK();

   return 1;
}



/* _xwin_replace_vtable:
 *  Replace entries in vtable with magical wrappers.
 */
void _xwin_replace_vtable(struct GFX_VTABLE *vtable)
{
   memcpy(&_xwin_vtable, vtable, sizeof(GFX_VTABLE));

   vtable->acquire = _xwin_lock;
   vtable->release = _xwin_unlock;
   vtable->putpixel = _xwin_putpixel;
   vtable->vline = _xwin_vline;
   vtable->hline = _xwin_hline;
   vtable->hfill = _xwin_hline;
   vtable->rectfill = _xwin_rectfill;
   vtable->draw_sprite = _xwin_draw_sprite;
   vtable->draw_sprite_ex = _xwin_draw_sprite_ex;
   vtable->draw_256_sprite = _xwin_draw_256_sprite;
   vtable->draw_sprite_v_flip = _xwin_draw_sprite_v_flip;
   vtable->draw_sprite_h_flip = _xwin_draw_sprite_h_flip;
   vtable->draw_sprite_vh_flip = _xwin_draw_sprite_vh_flip;
   vtable->draw_trans_sprite = _xwin_draw_trans_sprite;
   vtable->draw_trans_rgba_sprite = _xwin_draw_trans_rgba_sprite;
   vtable->draw_lit_sprite = _xwin_draw_lit_sprite;
   vtable->draw_rle_sprite = _xwin_draw_rle_sprite;
   vtable->draw_trans_rle_sprite = _xwin_draw_trans_rle_sprite;
   vtable->draw_trans_rgba_rle_sprite = _xwin_draw_trans_rgba_rle_sprite;
   vtable->draw_lit_rle_sprite = _xwin_draw_lit_rle_sprite;
   vtable->draw_character = _xwin_draw_character;
   vtable->draw_glyph = _xwin_draw_glyph;
   vtable->blit_from_memory = _xwin_blit_anywhere;
   vtable->blit_from_system = _xwin_blit_anywhere;
   vtable->blit_to_self = _xwin_blit_anywhere;
   vtable->blit_to_self_forward = _xwin_blit_anywhere;
   vtable->blit_to_self_backward = _xwin_blit_backward;
   vtable->masked_blit = _xwin_masked_blit;
   vtable->clear_to_color = _xwin_clear_to_color;
}



/* _xwin_lock:
 *  Lock X for drawing onto the display.
 */
void _xwin_lock(BITMAP *bmp)
{
   XLOCK ();
}



/* _xwin_lock:
 *  Unlock X after drawing. This will only truely call XUNLOCK when the
 *  unlock count reaches 0.
 */
void _xwin_unlock(BITMAP *bmp)
{
   XUNLOCK();
}



/* _xwin_update_video_bitmap:
 *  Update screen with correct offset for sub-bitmap.
 */
static void _xwin_update_video_bitmap(BITMAP *dst, int x, int y, int w, int h)
{
   _xwin_update_screen(x + dst->x_ofs, y + dst->y_ofs, w, h);
}



/* _xwin_putpixel:
 *  Wrapper for putpixel.
 */
static void _xwin_putpixel(BITMAP *dst, int dx, int dy, int color)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.putpixel(dst, dx, dy, color);
      return;
   }

   if (dst->clip && ((dx < dst->cl) || (dx >= dst->cr) || (dy < dst->ct) || (dy >= dst->cb)))
      return;

   _xwin_in_gfx_call = 1;
   _xwin_vtable.putpixel(dst, dx, dy, color);
   _xwin_in_gfx_call = 0;

   if (_xwin_direct_putpixel(dst, dx, dy, color))
      return;

   _xwin_update_video_bitmap(dst, dx, dy, 1, 1);
}



/* _xwin_hline:
 *  Wrapper for hline.
 */
static void _xwin_hline(BITMAP *dst, int dx1, int dy, int dx2, int color)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.hline(dst, dx1, dy, dx2, color);
      return;
   }

   if (dx1 > dx2) {
      int tmp = dx1;
      dx1 = dx2;
      dx2 = tmp;
   }

   if (dst->clip) {
      if (dx1 < dst->cl)
	 dx1 = dst->cl;
      if (dx2 >= dst->cr)
	 dx2 = dst->cr - 1;
      if ((dx1 > dx2) || (dy < dst->ct) || (dy >= dst->cb))
	 return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.hline(dst, dx1, dy, dx2, color);
   _xwin_in_gfx_call = 0;

   if (_xwin_direct_hline(dst, dx1, dy, dx2, color))
      return;

   _xwin_update_video_bitmap(dst, dx1, dy, dx2 - dx1 + 1, 1);
}



/* _xwin_vline:
 *  Wrapper for vline.
 */
static void _xwin_vline(BITMAP *dst, int dx, int dy1, int dy2, int color)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.vline(dst, dx, dy1, dy2, color);
      return;
   }

   if (dy1 > dy2) {
      int tmp = dy1;
      dy1 = dy2;
      dy2 = tmp;
   }

   if (dst->clip) {
      if (dy1 < dst->ct)
	 dy1 = dst->ct;
      if (dy2 >= dst->cb)
	 dy2 = dst->cb - 1;
      if ((dx < dst->cl) || (dx >= dst->cr) || (dy1 > dy2))
	 return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.vline(dst, dx, dy1, dy2, color);
   _xwin_in_gfx_call = 0;

   if (_xwin_direct_vline(dst, dx, dy1, dy2, color))
      return;

   _xwin_update_video_bitmap(dst, dx, dy1, 1, dy2 - dy1 + 1);
}



/* _xwin_rectfill:
 *  Wrapper for rectfill.
 */
static void _xwin_rectfill(BITMAP *dst, int dx1, int dy1, int dx2, int dy2, int color)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.rectfill(dst, dx1, dy1, dx2, dy2, color);
      return;
   }

   if (dy1 > dy2) {
      int tmp = dy1;
      dy1 = dy2;
      dy2 = tmp;
   }

   if (dx1 > dx2) {
      int tmp = dx1;
      dx1 = dx2;
      dx2 = tmp;
   }

   if (dst->clip) {
      if (dx1 < dst->cl)
	 dx1 = dst->cl;
      if (dx2 >= dst->cr)
	 dx2 = dst->cr - 1;
      if (dx1 > dx2)
	 return;

      if (dy1 < dst->ct)
	 dy1 = dst->ct;
      if (dy2 >= dst->cb)
	 dy2 = dst->cb - 1;
      if (dy1 > dy2)
	 return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.rectfill(dst, dx1, dy1, dx2, dy2, color);
   _xwin_in_gfx_call = 0;

   if (_xwin_direct_rectfill(dst, dx1, dy1, dx2, dy2, color))
      return;

   _xwin_update_video_bitmap(dst, dx1, dy1, dx2 - dx1 + 1, dy2 - dy1 + 1);
}



/* _xwin_clear_to_color:
 *  Wrapper for clear_to_color.
 */
static void _xwin_clear_to_color(BITMAP *dst, int color)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.clear_to_color(dst, color);
      return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.clear_to_color(dst, color);
   _xwin_in_gfx_call = 0;

   if (_xwin_direct_clear_to_color(dst, color))
      return;

   _xwin_update_video_bitmap(dst, dst->cl, dst->ct, dst->cr - dst->cl, dst->cb - dst->ct);
}



/* _xwin_blit_anywhere:
 *  Wrapper for blit.
 */
static void _xwin_blit_anywhere(BITMAP *src, BITMAP *dst, int sx, int sy,
				int dx, int dy, int w, int h)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.blit_from_memory(src, dst, sx, sy, dx, dy, w, h);
      return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.blit_from_memory(src, dst, sx, sy, dx, dy, w, h);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dx, dy, w, h);
}



/* _xwin_blit_backward:
 *  Wrapper for blit_backward.
 */
static void _xwin_blit_backward(BITMAP *src, BITMAP *dst, int sx, int sy,
				int dx, int dy, int w, int h)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.blit_to_self_backward(src, dst, sx, sy, dx, dy, w, h);
      return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.blit_to_self_backward(src, dst, sx, sy, dx, dy, w, h);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dx, dy, w, h);
}



/* _xwin_masked_blit:
 *  Wrapper for masked_blit.
 */
static void _xwin_masked_blit(BITMAP *src, BITMAP *dst, int sx, int sy,
			      int dx, int dy, int w, int h)
{
   if (_xwin_in_gfx_call) {
      _xwin_vtable.masked_blit(src, dst, sx, sy, dx, dy, w, h);
      return;
   }

   _xwin_in_gfx_call = 1;
   _xwin_vtable.masked_blit(src, dst, sx, sy, dx, dy, w, h);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dx, dy, w, h);
}



#define CLIP_BOX(dst, x, y, w, h, x_orig, y_orig, w_orig, h_orig)  \
{                                                                  \
   if (dst->clip) {                                                \
      int tmp, x_delta, y_delta;                                   \
                                                                   \
      tmp = dst->cl - x_orig;                                      \
      x_delta = ((tmp < 0) ? 0 : tmp);                             \
      x = x_orig + x_delta;                                        \
                                                                   \
      tmp = dst->cr - x_orig;                                      \
      w = ((tmp > w_orig) ? w_orig : tmp) - x_delta;               \
      if (w <= 0)                                                  \
	 return;                                                   \
                                                                   \
      tmp = dst->ct - y_orig;                                      \
      y_delta = ((tmp < 0) ? 0 : tmp);                             \
      y = y_orig + y_delta;                                        \
                                                                   \
      tmp = dst->cb - y_orig;                                      \
      h = ((tmp > h_orig) ? h_orig : tmp) - y_delta;               \
      if (h <= 0)                                                  \
	 return;                                                   \
   }                                                               \
   else {                                                          \
      x = x_orig;                                                  \
      y = y_orig;                                                  \
      w = w_orig;                                                  \
      h = h_orig;                                                  \
   }                                                               \
}



/* _xwin_draw_sprite:
 *  Wrapper for draw_sprite.
 */
static void _xwin_draw_sprite(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}

/* _xwin_draw_sprite_ex:
 *  Wrapper for draw_sprite_ex.
 */
static void _xwin_draw_sprite_ex(BITMAP *dst, BITMAP *src, int dx, int dy, int mode, int flip)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_sprite_ex(dst, src, dx, dy, mode, flip);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_sprite_ex(dst, src, dx, dy, mode, flip);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}

/* _xwin_draw_256_sprite:
 *  Wrapper for draw_256_sprite.
 */
static void _xwin_draw_256_sprite(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_256_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_256_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_sprite_v_flip:
 *  Wrapper for draw_sprite_v_flip.
 */
static void _xwin_draw_sprite_v_flip(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_sprite_v_flip(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_sprite_v_flip(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_sprite_h_flip:
 *  Wrapper for draw_sprite_h_flip.
 */
static void _xwin_draw_sprite_h_flip(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_sprite_h_flip(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_sprite_h_flip(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_sprite_vh_flip:
 *  Wrapper for draw_sprite_vh_flip.
 */
static void _xwin_draw_sprite_vh_flip(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_sprite_vh_flip(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_sprite_vh_flip(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}

/* _xwin_draw_trans_sprite:
 *  Wrapper for draw_trans_sprite.
 */
static void _xwin_draw_trans_sprite(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_trans_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_trans_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_trans_rgba_sprite:
 *  Wrapper for draw_trans_rgba_sprite.
 */
static void _xwin_draw_trans_rgba_sprite(BITMAP *dst, BITMAP *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_trans_rgba_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_trans_rgba_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_lit_sprite:
 *  Wrapper for draw_lit_sprite.
 */
static void _xwin_draw_lit_sprite(BITMAP *dst, BITMAP *src, int dx, int dy, int color)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_lit_sprite(dst, src, dx, dy, color);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_lit_sprite(dst, src, dx, dy, color);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_character:
 *  Wrapper for draw_character.
 */
static void _xwin_draw_character(BITMAP *dst, BITMAP *src, int dx, int dy, int color, int bg)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_character(dst, src, dx, dy, color, bg);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_character(dst, src, dx, dy, color, bg);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_glyph:
 *  Wrapper for draw_glyph.
 */
static void _xwin_draw_glyph(BITMAP *dst, AL_CONST FONT_GLYPH *src, int dx, int dy, int color, int bg)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_glyph(dst, src, dx, dy, color, bg);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_glyph(dst, src, dx, dy, color, bg);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_rle_sprite:
 *  Wrapper for draw_rle_sprite.
 */
static void _xwin_draw_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_rle_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_rle_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_trans_rle_sprite:
 *  Wrapper for draw_trans_rle_sprite.
 */
static void _xwin_draw_trans_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_trans_rle_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_trans_rle_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_trans_rgba_rle_sprite:
 *  Wrapper for draw_trans_rgba_rle_sprite.
 */
static void _xwin_draw_trans_rgba_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_trans_rgba_rle_sprite(dst, src, dx, dy);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_trans_rgba_rle_sprite(dst, src, dx, dy);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}



/* _xwin_draw_lit_rle_sprite:
 *  Wrapper for draw_lit_rle_sprite.
 */
static void _xwin_draw_lit_rle_sprite(BITMAP *dst, AL_CONST RLE_SPRITE *src, int dx, int dy, int color)
{
   int dxbeg, dybeg, w, h;

   if (_xwin_in_gfx_call) {
      _xwin_vtable.draw_lit_rle_sprite(dst, src, dx, dy, color);
      return;
   }

   CLIP_BOX(dst, dxbeg, dybeg, w, h, dx, dy, src->w, src->h)

   _xwin_in_gfx_call = 1;
   _xwin_vtable.draw_lit_rle_sprite(dst, src, dx, dy, color);
   _xwin_in_gfx_call = 0;
   _xwin_update_video_bitmap(dst, dxbeg, dybeg, w, h);
}
