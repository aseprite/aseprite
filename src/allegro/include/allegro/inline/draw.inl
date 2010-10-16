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
 *      Draw inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DRAW_INL
#define ALLEGRO_DRAW_INL

#include "allegro/debug.h"
#include "allegro/3d.h"
#include "gfx.inl"

#ifdef __cplusplus
   extern "C" {
#endif

AL_INLINE(int, getpixel, (BITMAP *bmp, int x, int y),
{
   ASSERT(bmp);

   return bmp->vtable->getpixel(bmp, x, y);
})


AL_INLINE(void, putpixel, (BITMAP *bmp, int x, int y, int color),
{
   ASSERT(bmp);

   bmp->vtable->putpixel(bmp, x, y, color);
})


AL_INLINE(void, _allegro_vline, (BITMAP *bmp, int x, int y_1, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->vline(bmp, x, y_1, y2, color);
})


AL_INLINE(void, _allegro_hline, (BITMAP *bmp, int x1, int y, int x2, int color),
{
   ASSERT(bmp);

   bmp->vtable->hline(bmp, x1, y, x2, color);
})


/* The curses API also contains functions called vline and hline so we have
 * called our functions _allegro_vline and _allegro_hline.  User programs
 * should use the vline/hline aliases as they are the official names.
 */
#ifndef ALLEGRO_NO_VHLINE_ALIAS
   AL_ALIAS_VOID_RET(vline(BITMAP *bmp, int x, int y_1, int y2, int color), _allegro_vline(bmp, x, y_1, y2, color))
   AL_ALIAS_VOID_RET(hline(BITMAP *bmp, int x1, int y, int x2, int color),  _allegro_hline(bmp, x1, y, x2, color))
#endif


AL_INLINE(void, line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->line(bmp, x1, y_1, x2, y2, color);
})


AL_INLINE(void, fastline, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->fastline(bmp, x1, y_1, x2, y2, color);
})


AL_INLINE(void, rectfill, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->rectfill(bmp, x1, y_1, x2, y2, color);
})


AL_INLINE(void, triangle, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int x3, int y3, int color),
{
   ASSERT(bmp);

   bmp->vtable->triangle(bmp, x1, y_1, x2, y2, x3, y3, color);
})


AL_INLINE(void, polygon, (BITMAP *bmp, int vertices, AL_CONST int *points, int color),
{
   ASSERT(bmp);

   bmp->vtable->polygon(bmp, vertices, points, color);
})


AL_INLINE(void, rect, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->rect(bmp, x1, y_1, x2, y2, color);
})


AL_INLINE(void, circle, (BITMAP *bmp, int x, int y, int radius, int color),
{
   ASSERT(bmp);

   bmp->vtable->circle(bmp, x, y, radius, color);
})


AL_INLINE(void, circlefill, (BITMAP *bmp, int x, int y, int radius, int color),
{
   ASSERT(bmp);

   bmp->vtable->circlefill(bmp, x, y, radius, color);
})



AL_INLINE(void, ellipse, (BITMAP *bmp, int x, int y, int rx, int ry, int color),
{
   ASSERT(bmp);

   bmp->vtable->ellipse(bmp, x, y, rx, ry, color);
})



AL_INLINE(void, ellipsefill, (BITMAP *bmp, int x, int y, int rx, int ry, int color),
{
   ASSERT(bmp);

   bmp->vtable->ellipsefill(bmp, x, y, rx, ry, color);
})



AL_INLINE(void, arc, (BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color),
{
   ASSERT(bmp);

   bmp->vtable->arc(bmp, x, y, ang1, ang2, r, color);
})



AL_INLINE(void, spline, (BITMAP *bmp, AL_CONST int points[8], int color),
{
   ASSERT(bmp);

   bmp->vtable->spline(bmp, points, color);
})



AL_INLINE(void, floodfill, (BITMAP *bmp, int x, int y, int color),
{
   ASSERT(bmp);

   bmp->vtable->floodfill(bmp, x, y, color);
})



AL_INLINE(void, polygon3d, (BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[]),
{
   ASSERT(bmp);

   bmp->vtable->polygon3d(bmp, type, texture, vc, vtx);
})



AL_INLINE(void, polygon3d_f, (BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[]),
{
   ASSERT(bmp);

   bmp->vtable->polygon3d_f(bmp, type, texture, vc, vtx);
})



AL_INLINE(void, triangle3d, (BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3),
{
   ASSERT(bmp);

   bmp->vtable->triangle3d(bmp, type, texture, v1, v2, v3);
})



AL_INLINE(void, triangle3d_f, (BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3),
{
   ASSERT(bmp);

   bmp->vtable->triangle3d_f(bmp, type, texture, v1, v2, v3);
})



AL_INLINE(void, quad3d, (BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4),
{
   ASSERT(bmp);

   bmp->vtable->quad3d(bmp, type, texture, v1, v2, v3, v4);
})



AL_INLINE(void, quad3d_f, (BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4),
{
   ASSERT(bmp);

   bmp->vtable->quad3d_f(bmp, type, texture, v1, v2, v3, v4);
})





AL_INLINE(void, draw_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 8) {
      bmp->vtable->draw_256_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);
      bmp->vtable->draw_sprite(bmp, sprite, x, y);
   }
})

AL_INLINE(void, draw_sprite_ex, (BITMAP *bmp, BITMAP *sprite, int x, int y,
                                 int mode, int flip),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (mode == DRAW_SPRITE_TRANS) {
      ASSERT((bmp->vtable->color_depth == sprite->vtable->color_depth) ||
             (sprite->vtable->color_depth == 32) ||
            ((sprite->vtable->color_depth == 8) &&
             (bmp->vtable->color_depth == 32)));
      bmp->vtable->draw_sprite_ex(bmp, sprite, x, y, mode, flip);
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);
      bmp->vtable->draw_sprite_ex(bmp, sprite, x, y, mode, flip);
   }
})


AL_INLINE(void, draw_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_v_flip(bmp, sprite, x, y);
})

AL_INLINE(void, draw_sprite_h_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_h_flip(bmp, sprite, x, y);
})

AL_INLINE(void, draw_sprite_vh_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_vh_flip(bmp, sprite, x, y);
})

AL_INLINE(void, draw_trans_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 32) {
      ASSERT(bmp->vtable->draw_trans_rgba_sprite);
      bmp->vtable->draw_trans_rgba_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT((bmp->vtable->color_depth == sprite->vtable->color_depth) ||
             ((bmp->vtable->color_depth == 32) &&
              (sprite->vtable->color_depth == 8)));
      bmp->vtable->draw_trans_sprite(bmp, sprite, x, y);
   }
})


AL_INLINE(void, draw_lit_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_lit_sprite(bmp, sprite, x, y, color);
})


AL_INLINE(void, draw_gouraud_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_gouraud_sprite(bmp, sprite, x, y, c1, c2, c3, c4);
})


AL_INLINE(void, draw_character_ex, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(sprite->vtable->color_depth == 8);

   bmp->vtable->draw_character(bmp, sprite, x, y, color, bg);
})


AL_INLINE(void, rotate_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
			     			      (y<<16) + (sprite->h * 0x10000) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, 0x10000, FALSE);
})


AL_INLINE(void, rotate_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
			     			      (y<<16) + (sprite->h * 0x10000) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, 0x10000, TRUE);
})


AL_INLINE(void, rotate_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
			     			      (y<<16) + (sprite->h * scale) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, scale, FALSE);
})


AL_INLINE(void, rotate_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
			     			      (y<<16) + (sprite->h * scale) / 2,
			     			      sprite->w << 15, sprite->h << 15,
			     			      angle, scale, TRUE);
})


AL_INLINE(void, pivot_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, FALSE);
})


AL_INLINE(void, pivot_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, TRUE);
})


AL_INLINE(void, pivot_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, FALSE);
})


AL_INLINE(void, pivot_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale),
{
   ASSERT(bmp);
   ASSERT(sprite);

   bmp->vtable->pivot_scaled_sprite_flip(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, TRUE);
})


AL_INLINE(void, _putpixel, (BITMAP *bmp, int x, int y, int color),
{
   uintptr_t addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write8(addr+x, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel, (BITMAP *bmp, int x, int y),
{
   uintptr_t addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read8(addr+x);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel15, (BITMAP *bmp, int x, int y, int color),
{
   uintptr_t addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write15(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel15, (BITMAP *bmp, int x, int y),
{
   uintptr_t addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read15(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel16, (BITMAP *bmp, int x, int y, int color),
{
   uintptr_t addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write16(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel16, (BITMAP *bmp, int x, int y),
{
   uintptr_t addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read16(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel24, (BITMAP *bmp, int x, int y, int color),
{
   uintptr_t addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write24(addr+x*3, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel24, (BITMAP *bmp, int x, int y),
{
   uintptr_t addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read24(addr+x*3);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel32, (BITMAP *bmp, int x, int y, int color),
{
   uintptr_t addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write32(addr+x*sizeof(int32_t), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel32, (BITMAP *bmp, int x, int y),
{
   uintptr_t addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read32(addr+x*sizeof(int32_t));
   bmp_unwrite_line(bmp);

   return c;
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DRAW_INL */


