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
 *      Font loading routine for reading a font from a datafile.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

/* load_dat_font:
 *  Loads a font from the datafile named filename. param can optionally be set
 *  to a list of two strings, which you can use to load a specific font and
 *  palette from a datafile by name. If pal is NULL, or the second string
 *  argument is NULL, no palette information is returned.
 *  If param is NULL, the first font found in the datafile will be returned
 *  and the last palette found prior to the font will be returned.
 */
FONT *load_dat_font(AL_CONST char *filename, RGB *pal, void *param)
{
   FONT *fnt;
   DATAFILE *df;
   RGB *p = NULL;
   char **names;
   int c, want_palette;
   ASSERT(filename);
   
   /* Load font and palette by name? */
   names = param;
   
   fnt = NULL;
   p = NULL;

   /* Load font by name? */
   if (names && names[0]) {
      df = load_datafile_object(filename, names[0]);
      if (!df) {
         return NULL;
      }
      fnt = df->dat;
      df->dat = NULL;
      unload_datafile_object(df);
   }
   
   /* Load palette by name? */
   want_palette = TRUE;
   if (names && names[1]) {
      df = load_datafile_object(filename, names[1]);
      if (df) {
         memcpy(pal, df->dat, sizeof(PALETTE));
      }
      unload_datafile_object(df);
      want_palette = FALSE;
   }
   
   if (!fnt || want_palette) {
      df = load_datafile(filename);
   
      if (!df) {
         return NULL;
      }
   
      for (c=0; df[c].type!=DAT_END; c++) {
         /* Only pick the last palette if no palette was named */
         if (df[c].type==DAT_PALETTE && want_palette)
            p = df[c].dat;
         /* Only pick a font if no font was named */
         if (df[c].type==DAT_FONT && !fnt) {
            fnt = df[c].dat;
            df[c].dat = NULL;
            break;
         }
      }
   
   
      if (p && pal && want_palette && fnt) {
         memcpy(pal, p, sizeof(PALETTE));
      }
   
      unload_datafile(df);
   }
   
   return fnt;
}
