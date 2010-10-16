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
 *
 *      GRX font file reader by Mark Wodrich.
 *
 *      See readme.txt for copyright information.
 */
#include "allegro.h"
#include "allegro/internal/aintern.h"

/* magic number for GRX format font files */
#define FONTMAGIC    0x19590214L



/* load_grx_font:
 *  Loads a GRX font from the file named filename. The palette and param
 *  parameters are ignored by this function.
 */
FONT *load_grx_font(AL_CONST char *filename, RGB *pal, void *param)
{
   PACKFILE *pack;
   FONT *f;
   FONT_MONO_DATA *mf;
   FONT_GLYPH **gl;
   int w, h, num, i;
   int* wtab = 0;
   ASSERT(filename);
   pack = pack_fopen(filename, F_READ);
   if (!pack) return NULL;

   if (pack_igetl(pack) != FONTMAGIC) {
      pack_fclose(pack);
      return NULL;
   }
   pack_igetl(pack);

   f = _AL_MALLOC(sizeof(FONT));
   mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));

   f->data = mf;
   f->vtable = font_vtable_mono;
   mf->next = NULL;

   w = pack_igetw(pack);
   h = pack_igetw(pack);

   f->height = h;

   mf->begin = pack_igetw(pack);
   mf->end = pack_igetw(pack) + 1;
   num = mf->end - mf->begin;

   gl = mf->glyphs = _AL_MALLOC(sizeof(FONT_GLYPH*) * num);

   if (pack_igetw(pack) == 0) {
      for (i = 0; i < 38; i++) pack_getc(pack);
      wtab = _AL_MALLOC_ATOMIC(sizeof(int) * num);
      for (i = 0; i < num; i++) wtab[i] = pack_igetw(pack);
   }
   else {
      for (i = 0; i < 38; i++) pack_getc(pack);
   }

   for(i = 0; i < num; i++) {
      int sz;

      if (wtab) w = wtab[i];

      sz = ((w + 7) / 8) * h;
      gl[i] = _AL_MALLOC(sizeof(FONT_GLYPH) + sz);
      gl[i]->w = w;
      gl[i]->h = h;

      pack_fread(gl[i]->dat, sz, pack);
   }

   pack_fclose(pack);
   if (wtab) _AL_FREE(wtab);

   return f;
}



/* load_grx_or_bios_font:
 *  Loads a GRX or bios font from the file named filename.
 */
FONT *load_grx_or_bios_font(AL_CONST char *filename, RGB *pal, void *param)
{
   PACKFILE *f;
   FONT *font = NULL;
   char tmp[16];
   int id;
   ASSERT(filename);

   if (ustricmp(get_extension(filename), uconvert_ascii("fnt", tmp)) == 0) {
      f = pack_fopen(filename, F_READ);
      if (!f)
	 return NULL;

      id = pack_igetl(f);
      pack_fclose(f);

      if (id == FONTMAGIC)
	 font = load_grx_font(filename, pal, param);
      else
	 font = load_bios_font(filename, pal, param);
   }
   
   return font;
}
