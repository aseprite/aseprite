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
 *      Font loading routine for reading a BIOS font.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

/* load_bios_font:
 *  Loads a PC BIOS font from the file named filename. The palette and param
 *  parameters are ignored by this function.
 */
FONT *load_bios_font(AL_CONST char *filename, RGB *pal, void *param)
{
    PACKFILE *pack;
    FONT *f;
    FONT_MONO_DATA *mf;
    FONT_GLYPH **gl;
    int i, h;

    f = _AL_MALLOC(sizeof(FONT));
    mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));
    gl = _AL_MALLOC(sizeof(FONT_GLYPH*) * 256);

    pack = pack_fopen(filename, F_READ);
    if (!pack) return 0;

    h = (pack->normal.todo == 2048) ? 8 : 16;

    for (i = 0; i < 256; i++) {
        gl[i] = _AL_MALLOC(sizeof(FONT_GLYPH) + h);
        gl[i]->w = 8;
        gl[i]->h = h;
        pack_fread(gl[i]->dat, h, pack);
    }

    f->vtable = font_vtable_mono;
    f->data = mf;
    f->height = h;

    mf->begin = 0;
    mf->end = 256;
    mf->glyphs = gl;
    mf->next = 0;

    pack_fclose(pack);

    return f;
}
