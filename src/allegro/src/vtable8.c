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
 *      Table of functions for drawing onto 8 bit linear bitmaps.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#ifdef ALLEGRO_COLOR8


void _linear_draw_sprite8_end(void);
void _linear_blit8_end(void);


GFX_VTABLE __linear_vtable8 =
{
   8,
   MASK_COLOR_8,
   _stub_unbank_switch,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _linear_getpixel8,
   _linear_putpixel8,
   _linear_vline8,
   _linear_hline8,
   _linear_hline8,
   _normal_line,
   _fast_line,
   _normal_rectfill,
   _soft_triangle,
   _linear_draw_sprite8,
   _linear_draw_sprite8,
   _linear_draw_sprite_v_flip8,
   _linear_draw_sprite_h_flip8,
   _linear_draw_sprite_vh_flip8,
   _linear_draw_trans_sprite8,
   NULL,
   _linear_draw_lit_sprite8,
   _linear_draw_rle_sprite8,
   _linear_draw_trans_rle_sprite8,
   NULL,
   _linear_draw_lit_rle_sprite8,
   _linear_draw_character8,
   _linear_draw_glyph8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit8,
   _linear_blit_backward8,
   _blit_between_formats,
   _linear_masked_blit8,
   _linear_clear_to_color8,
   _pivot_scaled_sprite_flip,
   NULL,    // AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked))
   _soft_draw_gouraud_sprite,
   _linear_draw_sprite8_end,
   _linear_blit8_end,
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
   _linear_draw_sprite_ex8
};


#endif

