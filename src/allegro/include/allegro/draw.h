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
 *      Drawing and sprite routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DRAW_H
#define ALLEGRO_DRAW_H

#include "base.h"
#include "fixed.h"
#include "gfx.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define DRAW_MODE_SOLID             0        /* flags for drawing_mode() */
#define DRAW_MODE_XOR               1
#define DRAW_MODE_COPY_PATTERN      2
#define DRAW_MODE_SOLID_PATTERN     3
#define DRAW_MODE_MASKED_PATTERN    4
#define DRAW_MODE_TRANS             5

AL_FUNC(void, drawing_mode, (int mode, struct BITMAP *pattern, int x_anchor, int y_anchor));
AL_FUNC(void, xor_mode, (int on));
AL_FUNC(void, solid_mode, (void));
AL_FUNC(void, do_line, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int d, AL_METHOD(void, proc, (struct BITMAP *, int, int, int))));
AL_FUNC(void, _soft_triangle, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int x3, int y3, int color));
AL_FUNC(void, _soft_polygon, (struct BITMAP *bmp, int vertices, AL_CONST int *points, int color));
AL_FUNC(void, _soft_rect, (struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
AL_FUNC(void, do_circle, (struct BITMAP *bmp, int x, int y, int radius, int d, AL_METHOD(void, proc, (struct BITMAP *, int, int, int))));
AL_FUNC(void, _soft_circle, (struct BITMAP *bmp, int x, int y, int radius, int color));
AL_FUNC(void, _soft_circlefill, (struct BITMAP *bmp, int x, int y, int radius, int color));
AL_FUNC(void, do_ellipse, (struct BITMAP *bmp, int x, int y, int rx, int ry, int d, AL_METHOD(void, proc, (struct BITMAP *, int, int, int))));
AL_FUNC(void, _soft_ellipse, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
AL_FUNC(void, _soft_ellipsefill, (struct BITMAP *bmp, int x, int y, int rx, int ry, int color));
AL_FUNC(void, do_arc, (struct BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int d, AL_METHOD(void, proc, (struct BITMAP *, int, int, int))));
AL_FUNC(void, _soft_arc, (struct BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color));
AL_FUNC(void, calc_spline, (AL_CONST int points[8], int npts, int *x, int *y));
AL_FUNC(void, _soft_spline, (struct BITMAP *bmp, AL_CONST int points[8], int color));
AL_FUNC(void, _soft_floodfill, (struct BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, masked_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, stretch_blit, (struct BITMAP *s, struct BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h));
AL_FUNC(void, masked_stretch_blit, (struct BITMAP *s, struct BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h));
AL_FUNC(void, stretch_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int w, int h));
AL_FUNC(void, _soft_draw_gouraud_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4));

/* rotate+trans */
AL_FUNC(void, rotate_sprite_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle));
AL_FUNC(void, rotate_sprite_v_flip_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle));
AL_FUNC(void, rotate_scaled_sprite_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale));
AL_FUNC(void, rotate_scaled_sprite_v_flip_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale));
AL_FUNC(void, pivot_sprite_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle));
AL_FUNC(void, pivot_sprite_v_flip_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle));
AL_FUNC(void, pivot_scaled_sprite_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale));
AL_FUNC(void, pivot_scaled_sprite_v_flip_trans, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale));
/* rotate+lit */
AL_FUNC(void, rotate_sprite_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, int color));
AL_FUNC(void, rotate_sprite_v_flip_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, int color));
AL_FUNC(void, rotate_scaled_sprite_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale, int color));
AL_FUNC(void, rotate_scaled_sprite_v_flip_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale, int color));
AL_FUNC(void, pivot_sprite_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, int color));
AL_FUNC(void, pivot_sprite_v_flip_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, int color));
AL_FUNC(void, pivot_scaled_sprite_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale, int color));
AL_FUNC(void, pivot_scaled_sprite_v_flip_lit, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale, int color));



#ifdef __cplusplus
   }
#endif

#include "inline/draw.inl"

#endif          /* ifndef ALLEGRO_DRAW_H */


