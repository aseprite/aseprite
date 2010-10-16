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
 *      Font loading routines.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FONT_H
#define ALLEGRO_FONT_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct FONT_GLYPH           /* a single monochrome font character */
{
   short w, h;
   ZERO_SIZE_ARRAY(unsigned char, dat);
} FONT_GLYPH;


struct FONT_VTABLE;

typedef struct FONT
{
   void *data;
   int height;
   struct FONT_VTABLE *vtable;
} FONT;

AL_FUNC(int, font_has_alpha, (FONT *f));
AL_FUNC(void, make_trans_font, (FONT *f));

AL_FUNC(int, is_trans_font, (FONT *f));
AL_FUNC(int, is_color_font, (FONT *f));
AL_FUNC(int, is_mono_font, (FONT *f));
AL_FUNC(int, is_compatible_font, (FONT *f1, FONT *f2));

AL_FUNC(void, register_font_file_type, (AL_CONST char *ext, FONT *(*load)(AL_CONST char *filename, RGB *pal, void *param)));
AL_FUNC(FONT *, load_font, (AL_CONST char *filename, RGB *pal, void *param));

AL_FUNC(FONT *, load_dat_font, (AL_CONST char *filename, RGB *pal, void *param));
AL_FUNC(FONT *, load_bios_font, (AL_CONST char *filename, RGB *pal, void *param));
AL_FUNC(FONT *, load_grx_font, (AL_CONST char *filename, RGB *pal, void *param));
AL_FUNC(FONT *, load_grx_or_bios_font, (AL_CONST char *filename, RGB *pal, void *param));
AL_FUNC(FONT *, load_bitmap_font, (AL_CONST char *fname, RGB *pal, void *param));
AL_FUNC(FONT *, load_txt_font, (AL_CONST char *fname, RGB *pal, void *param));

AL_FUNC(FONT *, grab_font_from_bitmap, (BITMAP *bmp));

AL_FUNC(int, get_font_ranges, (FONT *f));
AL_FUNC(int, get_font_range_begin, (FONT *f, int range));
AL_FUNC(int, get_font_range_end, (FONT *f, int range));
AL_FUNC(FONT *, extract_font_range, (FONT *f, int begin, int end));
AL_FUNC(FONT *, merge_fonts, (FONT *f1, FONT *f2));
AL_FUNC(int, transpose_font, (FONT *f, int drange));
#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FONT_H */


