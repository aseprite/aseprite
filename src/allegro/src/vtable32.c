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
 *      Table of functions for drawing onto 32 bit linear bitmaps.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#ifdef ALLEGRO_COLOR32


void _linear_draw_sprite32_end(void);
void _linear_blit32_end(void);


GFX_VTABLE __linear_vtable32 =
{
   32,
   MASK_COLOR_32,
   _stub_unbank_switch,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _linear_getpixel32,
   _linear_putpixel32,
   _linear_vline32,
   _linear_hline32,
   _linear_hline32,
   _normal_line,
   _fast_line,
   _normal_rectfill,
   _soft_triangle,
   _linear_draw_sprite32,
   _linear_draw_256_sprite32,
   _linear_draw_sprite_v_flip32,
   _linear_draw_sprite_h_flip32,
   _linear_draw_sprite_vh_flip32,
   _linear_draw_trans_sprite32,
   _linear_draw_trans_sprite32,
   _linear_draw_lit_sprite32,
   _linear_draw_rle_sprite32,
   _linear_draw_trans_rle_sprite32,
   _linear_draw_trans_rle_sprite32,
   _linear_draw_lit_rle_sprite32,
   _linear_draw_character32,
   _linear_draw_glyph32,
   _linear_blit32,
   _linear_blit32,
   _linear_blit32,
   _linear_blit32,
   _linear_blit32,
   _linear_blit32,
   _linear_blit_backward32,
   _blit_between_formats,
   _linear_masked_blit32,
   _linear_clear_to_color32,
   _pivot_scaled_sprite_flip,
   NULL,    // AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked))
   _soft_draw_gouraud_sprite,
   _linear_draw_sprite32_end,
   _linear_blit32_end,
   _soft_polygon,
   _soft_rect,
   _soft_circle,
   _soft_circlefill,
   _soft_ellipse,
   _soft_ellipsefill,
   _soft_arc,
   _soft_spline,
   _soft_floodfill,

   _soft_polygon3d,
   _soft_polygon3d_f,
   _soft_triangle3d,
   _soft_triangle3d_f,
   _soft_quad3d,
   _soft_quad3d_f,
   _linear_draw_sprite_ex32
};


#endif
