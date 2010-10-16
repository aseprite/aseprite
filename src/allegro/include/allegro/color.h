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
 *      Color manipulation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COLOR_H
#define ALLEGRO_COLOR_H

#include "base.h"
#include "palette.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;

AL_VAR(PALETTE, black_palette);
AL_VAR(PALETTE, desktop_palette);
AL_VAR(PALETTE, default_palette);

typedef struct {
   unsigned char data[32][32][32];
} RGB_MAP;

typedef struct {
   unsigned char data[PAL_SIZE][PAL_SIZE];
} COLOR_MAP;

AL_VAR(RGB_MAP *, rgb_map);
AL_VAR(COLOR_MAP *, color_map);

AL_VAR(PALETTE, _current_palette);

AL_VAR(int, _rgb_r_shift_15);
AL_VAR(int, _rgb_g_shift_15);
AL_VAR(int, _rgb_b_shift_15);
AL_VAR(int, _rgb_r_shift_16);
AL_VAR(int, _rgb_g_shift_16);
AL_VAR(int, _rgb_b_shift_16);
AL_VAR(int, _rgb_r_shift_24);
AL_VAR(int, _rgb_g_shift_24);
AL_VAR(int, _rgb_b_shift_24);
AL_VAR(int, _rgb_r_shift_32);
AL_VAR(int, _rgb_g_shift_32);
AL_VAR(int, _rgb_b_shift_32);
AL_VAR(int, _rgb_a_shift_32);

AL_ARRAY(int, _rgb_scale_5);
AL_ARRAY(int, _rgb_scale_6);

#define MASK_COLOR_8       0
#define MASK_COLOR_15      0x7C1F
#define MASK_COLOR_16      0xF81F
#define MASK_COLOR_24      0xFF00FF
#define MASK_COLOR_32      0xFF00FF

AL_VAR(int *, palette_color);

AL_FUNC(void, set_color, (int idx, AL_CONST RGB *p));
AL_FUNC(void, set_palette, (AL_CONST PALETTE p));
AL_FUNC(void, set_palette_range, (AL_CONST PALETTE p, int from, int to, int retracesync));

AL_FUNC(void, get_color, (int idx, RGB *p));
AL_FUNC(void, get_palette, (PALETTE p));
AL_FUNC(void, get_palette_range, (PALETTE p, int from, int to));

AL_FUNC(void, fade_interpolate, (AL_CONST PALETTE source, AL_CONST PALETTE dest, PALETTE output, int pos, int from, int to));
AL_FUNC(void, fade_from_range, (AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed, int from, int to));
AL_FUNC(void, fade_in_range, (AL_CONST PALETTE p, int speed, int from, int to));
AL_FUNC(void, fade_out_range, (int speed, int from, int to));
AL_FUNC(void, fade_from, (AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed));
AL_FUNC(void, fade_in, (AL_CONST PALETTE p, int speed));
AL_FUNC(void, fade_out, (int speed));

AL_FUNC(void, select_palette, (AL_CONST PALETTE p));
AL_FUNC(void, unselect_palette, (void));

AL_FUNC(void, generate_332_palette, (PALETTE pal));
AL_FUNC(int, generate_optimized_palette, (struct BITMAP *image, PALETTE pal, AL_CONST signed char rsvdcols[256]));

AL_FUNC(void, create_rgb_table, (RGB_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_light_table, (COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_trans_table, (COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_color_table, (COLOR_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, blend, (AL_CONST PALETTE pal, int x, int y, RGB *rgb)), AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_blender_table, (COLOR_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))));

typedef AL_METHOD(unsigned long, BLENDER_FUNC, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(void, set_blender_mode, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a));
AL_FUNC(void, set_blender_mode_ex, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a));

AL_FUNC(void, set_alpha_blender, (void));
AL_FUNC(void, set_write_alpha_blender, (void));
AL_FUNC(void, set_trans_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_add_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_burn_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_color_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_difference_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dissolve_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dodge_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_hue_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_invert_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_luminance_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_multiply_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_saturation_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_screen_blender, (int r, int g, int b, int a));

AL_FUNC(void, hsv_to_rgb, (float h, float s, float v, int *r, int *g, int *b));
AL_FUNC(void, rgb_to_hsv, (int r, int g, int b, float *h, float *s, float *v));

AL_FUNC(int, bestfit_color, (AL_CONST PALETTE pal, int r, int g, int b));

AL_FUNC(int, makecol, (int r, int g, int b));
AL_FUNC(int, makecol8, (int r, int g, int b));
AL_FUNC(int, makecol_depth, (int color_depth, int r, int g, int b));

AL_FUNC(int, makeacol, (int r, int g, int b, int a));
AL_FUNC(int, makeacol_depth, (int color_depth, int r, int g, int b, int a));

AL_FUNC(int, makecol15_dither, (int r, int g, int b, int x, int y));
AL_FUNC(int, makecol16_dither, (int r, int g, int b, int x, int y));

AL_FUNC(int, getr, (int c));
AL_FUNC(int, getg, (int c));
AL_FUNC(int, getb, (int c));
AL_FUNC(int, geta, (int c));

AL_FUNC(int, getr_depth, (int color_depth, int c));
AL_FUNC(int, getg_depth, (int color_depth, int c));
AL_FUNC(int, getb_depth, (int color_depth, int c));
AL_FUNC(int, geta_depth, (int color_depth, int c));

#ifdef __cplusplus
   }
#endif

#include "inline/color.inl"

#endif          /* ifndef ALLEGRO_COLOR_H */


