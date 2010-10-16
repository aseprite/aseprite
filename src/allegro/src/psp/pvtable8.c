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
 *      PSP screen gfx vtable for 8 bpp support.
 * 
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"


#define CONSTRUCT_8BPP_VERSION(type, func, args, params)                  \
   type psp_##func##8 args                                                \
   {                                                                      \
      bmp->vtable = &__linear_vtable8;                                    \
                                                                          \
      /* Call the original function. */                                   \
      __linear_vtable8.func params ;                                      \
                                                                          \
      if (is_same_bitmap(bmp, displayed_video_bitmap))                    \
         /* Show the screen texture. */                                   \
         psp_draw_to_screen();                                            \
                                                                          \
      bmp->vtable = &psp_vtable8;                                         \
   }


CONSTRUCT_8BPP_VERSION(void, putpixel, (BITMAP *bmp, int x, int y, int color), (bmp, x, y, color));
CONSTRUCT_8BPP_VERSION(void, vline, (BITMAP *bmp, int x, int y_1, int y2, int color), (bmp, x, y_1, y2, color));
CONSTRUCT_8BPP_VERSION(void, hline, (BITMAP *bmp, int x1, int y, int x2, int color), (bmp, x1, y, x2, color));
CONSTRUCT_8BPP_VERSION(void, hfill, (BITMAP *bmp, int x1, int y, int x2, int color), (bmp, x1, y, x2, color));
CONSTRUCT_8BPP_VERSION(void, line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color), (bmp, x1, y_1, x2, y2, color));
CONSTRUCT_8BPP_VERSION(void, fastline, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color), (bmp, x1, y_1, x2, y2, color));
CONSTRUCT_8BPP_VERSION(void, rectfill, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color), (bmp, x1, y_1, x2, y2, color));
CONSTRUCT_8BPP_VERSION(void, triangle, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int x3, int y3, int color), (bmp, x1, y_1, x2, y2, x3, y3, color));
CONSTRUCT_8BPP_VERSION(void, draw_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_256_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_sprite_h_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_sprite_vh_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_trans_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_lit_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color), (bmp, sprite, x, y, color));
CONSTRUCT_8BPP_VERSION(void, draw_rle_sprite, (BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_trans_rle_sprite, (BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y), (bmp, sprite, x, y));
CONSTRUCT_8BPP_VERSION(void, draw_lit_rle_sprite, (BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y, int color), (bmp, sprite, x, y, color));
CONSTRUCT_8BPP_VERSION(void, draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg), (bmp, sprite, x, y, color, bg));
CONSTRUCT_8BPP_VERSION(void, draw_glyph, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg), (bmp, glyph, x, y, color, bg));
CONSTRUCT_8BPP_VERSION(void, blit_from_memory, (BITMAP *source, BITMAP *bmp, int source_x, int source_y, int dest_x, int dest_y, int width, int height), (source, bmp, source_x, source_y, dest_x, dest_y, width, height));
CONSTRUCT_8BPP_VERSION(void, blit_from_system, (BITMAP *source, BITMAP *bmp, int source_x, int source_y, int dest_x, int dest_y, int width, int height), (source, bmp, source_x, source_y, dest_x, dest_y, width, height));
CONSTRUCT_8BPP_VERSION(void, blit_to_self, (BITMAP *source, BITMAP *bmp, int source_x, int source_y, int dest_x, int dest_y, int width, int height), (source, bmp, source_x, source_y, dest_x, dest_y, width, height));
CONSTRUCT_8BPP_VERSION(void, clear_to_color, (BITMAP *bmp, int color), (bmp, color));
CONSTRUCT_8BPP_VERSION(void, pivot_scaled_sprite_flip, (BITMAP *bmp, BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale, int v_flip), (bmp, sprite, x, y, cx, cy, angle, scale, v_flip));
CONSTRUCT_8BPP_VERSION(void, polygon, (BITMAP *bmp, int vertices, AL_CONST int *points, int color), (bmp, vertices, points, color));
CONSTRUCT_8BPP_VERSION(void, rect, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color), (bmp, x1, y_1, x2, y2, color));
CONSTRUCT_8BPP_VERSION(void, circle, (BITMAP *bmp, int x, int y, int radius, int color), (bmp, x, y, radius, color));
CONSTRUCT_8BPP_VERSION(void, circlefill, (BITMAP *bmp, int x, int y, int radius, int color), (bmp, x, y, radius, color));
CONSTRUCT_8BPP_VERSION(void, ellipse, (BITMAP *bmp, int x, int y, int rx, int ry, int color), (bmp, x, y, rx, ry, color));
CONSTRUCT_8BPP_VERSION(void, ellipsefill, (BITMAP *bmp, int x, int y, int rx, int ry, int color), (bmp, x, y, rx, ry, color));
CONSTRUCT_8BPP_VERSION(void, arc, (BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color), (bmp, x, y, ang1, ang2, r, color));
CONSTRUCT_8BPP_VERSION(void, spline, (BITMAP *bmp, AL_CONST int points[8], int color), (bmp, points, color));
CONSTRUCT_8BPP_VERSION(void, floodfill, (BITMAP *bmp, int x, int y, int color), (bmp, x, y, color));


/* psp_do_stretch_blit8:
 *  Hook to capture the call to stretch_blit().
 */
void psp_do_stretch_blit8(BITMAP *source, BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked)
{
   source->vtable->do_stretch_blit = NULL;

   stretch_blit(source, dest, source_x, source_y, source_width, source_height, dest_x, dest_y, dest_width, dest_height);
   if (is_same_bitmap(dest, displayed_video_bitmap))
      psp_draw_to_screen();

   source->vtable->do_stretch_blit = psp_do_stretch_blit8;
}



GFX_VTABLE psp_vtable8 =
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
   psp_putpixel8,
   psp_vline8,
   psp_hline8,
   psp_hfill8,
   psp_line8,
   psp_fastline8,
   psp_rectfill8,
   psp_triangle8,
   psp_draw_sprite8,
   psp_draw_256_sprite8,
   psp_draw_sprite_v_flip8,
   psp_draw_sprite_h_flip8,
   psp_draw_sprite_vh_flip8,
   psp_draw_trans_sprite8,
   NULL,
   psp_draw_lit_sprite8,
   psp_draw_rle_sprite8,
   psp_draw_trans_rle_sprite8,
   NULL,
   psp_draw_lit_rle_sprite8,
   psp_draw_character8,
   psp_draw_glyph8,
   psp_blit_from_memory8,
   _linear_blit8,
   psp_blit_from_system8,
   _linear_blit8,
   psp_blit_to_self8,
   NULL, //_blit_to_self_forward8,
   NULL, //_blit_to_self_backward8,
   NULL, //_blit_between_formats8,
   NULL, //_masked_blit8,
   psp_clear_to_color8,
   psp_pivot_scaled_sprite_flip8,
   NULL,
   NULL, //_draw_gouraud_sprite8,
   NULL, //_draw_sprite_end8,
   NULL, //_blit_end8,
   psp_polygon8,
   psp_rect8,
   psp_circle8,
   psp_circlefill8,
   psp_ellipse8,
   psp_ellipsefill8,
   psp_arc8,
   psp_spline8,
   psp_floodfill8,

   NULL, //_polygon3d8,
   NULL, //_polygon3d_f8,
   NULL, //_triangle3d8,
   NULL, //_triangle3d_f8,
   NULL, //_quad3d8,
   NULL, //_quad3d_f8,
   NULL //_draw_sprite_ex8
};
