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
 *      Top level font reading routines.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct FONT_TYPE_INFO
{
   char *ext;
   FONT *(*load)(AL_CONST char *filename, RGB *pal, void *param);
   struct FONT_TYPE_INFO *next;
} FONT_TYPE_INFO;

static FONT_TYPE_INFO *font_type_list = NULL;



/* register_font_file_type:
 *  Informs Allegro of a new font file type, telling it how to load files of 
 *  this format.
 */
void register_font_file_type(AL_CONST char *ext, FONT *(*load)(AL_CONST char *filename, RGB *pal, void *param))
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter = font_type_list;

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) 
      iter = font_type_list = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   else {
      for (iter = font_type_list; iter->next; iter = iter->next);
      iter = iter->next = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   }

   if (iter) {
      iter->load = load;
      iter->ext = _al_strdup(aext);
      iter->next = NULL;
   }
}



/* load_font:
 *  Loads a font from disk. Will try to load a font from a bitmap if all else
 *  fails.
 */
FONT *load_font(AL_CONST char *filename, RGB *pal, void *param)
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter;
   ASSERT(filename);

   aext = uconvert_toascii(get_extension(filename), tmp);
   
   for (iter = font_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->load)
	    return iter->load(filename, pal, param);
	 return NULL;
      }
   }
   
   /* Try to load the file as a bitmap image and grab the font from there */
   return load_bitmap_font(filename, pal, param);
}



/* register_font_file_type_exit:
 *  Free list of registered bitmap file types.
 */
static void register_font_file_type_exit(void)
{
   FONT_TYPE_INFO *iter = font_type_list, *next;

   while (iter) {
      next = iter->next;
      _AL_FREE(iter->ext);
      _AL_FREE(iter);
      iter = next;
   }
   
   font_type_list = NULL;

   /* If we are using a destructor, then we only want to prune the list
    * down to valid modules. So we clean up as usual, but then reinstall
    * the internal modules.
    */
   #ifdef ALLEGRO_USE_CONSTRUCTOR
      _register_font_file_type_init();
   #endif

   _remove_exit_func(register_font_file_type_exit);
}



/* _register_font_file_type_init:
 *  Register built-in font file types.
 */
void _register_font_file_type_init(void)
{
   char buf[32];

   _add_exit_func(register_font_file_type_exit,
		  "register_font_file_type_exit");

   register_font_file_type(uconvert_ascii("dat", buf), load_dat_font);
   register_font_file_type(uconvert_ascii("fnt", buf), load_grx_or_bios_font);
   register_font_file_type(uconvert_ascii("txt", buf), load_txt_font);
}



#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(static void font_filetype_constructor(void));
   DESTRUCTOR_FUNCTION(static void font_filetype_destructor(void));

   /* font_filetype_constructor:
    *  Register font filetype functions if this object file is linked
    *  in. This isn't called if the load_font() function isn't used 
    *  in a program, thus saving a little space in statically linked 
    *  programs.
    */
   void font_filetype_constructor(void)
   {
      _register_font_file_type_init();
   }

   /* font_filetype_destructor:
    *  Since we only want to destroy the whole list when we *actually*
    *  quit, not just when allegro_exit() is called, we need to use a
    *  destructor to accomplish this.
    */
   static void font_filetype_destructor(void)
   {
      FONT_TYPE_INFO *iter = font_type_list, *next;

      while (iter) {
         next = iter->next;
         _AL_FREE(iter->ext);
         _AL_FREE(iter);
         iter = next;
      }
   
      font_type_list = NULL;

      _remove_exit_func(register_font_file_type_exit);
   }
#endif
