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
 *      TXT font file reader.
 *
 *      See readme.txt for copyright information.
 */
#include <stdio.h>
#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"




/* load_txt_font:
 *  Loads a scripted font.
 */
FONT *load_txt_font(AL_CONST char *filename, RGB *pal, void *param)
{
   char buf[1024], *font_str, *start_str = 0, *end_str = 0;
   char font_filename[1024];
   FONT *f, *f2, *f3, *f4;
   PACKFILE *pack;
   int begin, end, glyph_pos=32;

   pack = pack_fopen(filename, F_READ);
   if (!pack) 
      return NULL;

   f = f2 = f3 = f4 = NULL;

   while(pack_fgets(buf, sizeof(buf)-1, pack)) {
      font_str = strtok(buf, " \t");
      if (font_str) 
         start_str = strtok(0, " \t");
      if (start_str) 
         end_str = strtok(0, " \t");

      if (!font_str || !start_str) {
         if (f)
            destroy_font(f);
         if (f2)
            destroy_font(f2);

         pack_fclose(pack);

         return NULL;
      }

      if(font_str[0] == '-')
         font_str[0] = '\0';

      begin = strtol(start_str, 0, 0);

      if (end_str)
         end = strtol(end_str, 0, 0);
      else 
         end = -1;

      if(begin <= 0 || (end > 0 && end < begin)) {
         if (f)
            destroy_font(f);
         if (f2)
            destroy_font(f2);

         pack_fclose(pack);

         return NULL;
      }

      /* Load the font that needs to be merged with the current font */
      if (font_str[0]) {
         if (f2)
            destroy_font(f2);
         if (exists(font_str)) {
            f2 = load_font(font_str, pal, param);
         } else if (is_relative_filename(font_str)) {
            replace_filename(font_filename, filename, font_str,
                             sizeof(font_filename));
            f2 = load_font(font_filename, pal, param);
         }
         else {
            f2 = NULL;
         }
         if (f2)
            glyph_pos=get_font_range_begin(f2, -1);
      }

      if (!f2) {
         if (f)
            destroy_font(f);
         pack_fclose(pack);
         return NULL;
      }

      if (end == -1)
         end = begin + get_font_range_end(f2,-1) - glyph_pos;

      /* transpose the font to the range given in the .txt file */
      f4=extract_font_range(f2,glyph_pos,glyph_pos + (end - begin));

      if (f4 && (begin != glyph_pos)) {
         transpose_font(f4, begin - glyph_pos);
      }
      glyph_pos += (end - begin) + 1;

      /* FIXME: More efficient way than to repeatedely merge into a new font? */
      if (f && f4) {
         f3 = f;
         f = merge_fonts(f4, f3);
         destroy_font(f4);
         destroy_font(f3);
      }
      else {
         f = f4;
      }
      f3=f4=NULL;
   }
   if (f2)
      destroy_font(f2);

   pack_fclose(pack);
   return f;
}
