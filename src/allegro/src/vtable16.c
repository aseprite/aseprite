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
 *      Table of functions for drawing onto 16 bit linear bitmaps.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#ifdef ALLEGRO_COLOR16


void _linear_draw_sprite16_end(void);
void _linear_blit16_end(void);


GFX_VTABLE __linear_vtable16 =
{
   16,
   MASK_COLOR_16,
   _stub_unbank_switch,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _linear_getpixel16,
   _linear_putpixel16,
   _linear_vline16,
   _linear_hline16,
   _linear_hline16,
   _normal_line,
   _fast_line,
   _normal_rectfill,
   _soft_triangle,
   _linear_draw_sprite16,
   _linear_draw_256_sprite16,
   _linear_draw_sprite_v_flip16,
   _linear_draw_sprite_h_flip16,
   _linear_draw_sprite_vh_flip16,
   _linear_draw_trans_sprite16,
   _linear_draw_trans_rgba_sprite16,
   _linear_draw_lit_sprite16,
   _linear_draw_rle_sprite16,
   _linear_draw_trans_rle_sprite16,
   _linear_draw_trans_rgba_rle_sprite16,
   _linear_draw_lit_rle_sprite16,
   _linear_draw_character16,
   _linear_draw_glyph16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit16,
   _linear_blit_backward16,
   _blit_between_formats,
   _linear_masked_blit16,
   _linear_clear_to_color16,
   _pivot_scaled_sprite_flip,
   NULL,    // AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked))
   _soft_draw_gouraud_sprite,
   _linear_draw_sprite16_end,
   _linear_blit16_end,
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
   _linear_draw_sprite_ex16
};


#endif

