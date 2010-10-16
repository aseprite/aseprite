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
 *      Top level bitmap reading routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct BITMAP_TYPE_INFO
{
   char *ext;
   BITMAP *(*load)(AL_CONST char *filename, RGB *pal);
   int (*save)(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal);
   struct BITMAP_TYPE_INFO *next;
} BITMAP_TYPE_INFO;

static BITMAP_TYPE_INFO *bitmap_type_list = NULL;



/* register_bitmap_file_type:
 *  Informs Allegro of a new image file type, telling it how to load and
 *  save files of this format (either function may be NULL).
 */
void register_bitmap_file_type(AL_CONST char *ext, BITMAP *(*load)(AL_CONST char *filename, RGB *pal), int (*save)(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal))
{
   char tmp[32], *aext;
   BITMAP_TYPE_INFO *iter = bitmap_type_list;

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) 
      iter = bitmap_type_list = _AL_MALLOC(sizeof(struct BITMAP_TYPE_INFO));
   else {
      for (iter = bitmap_type_list; iter->next; iter = iter->next);
      iter = iter->next = _AL_MALLOC(sizeof(struct BITMAP_TYPE_INFO));
   }

   if (iter) {
      iter->load = load;
      iter->save = save;
      iter->ext = _al_strdup(aext);
      iter->next = NULL;
   }
}



/* load_bitmap:
 *  Loads a bitmap from disk.
 */
BITMAP *load_bitmap(AL_CONST char *filename, RGB *pal)
{
   char tmp[32], *aext;
   BITMAP_TYPE_INFO *iter;
   ASSERT(filename);

   aext = uconvert_toascii(get_extension(filename), tmp);
   
   for (iter = bitmap_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->load)
	    return iter->load(filename, pal);
	 return NULL;
      }
   }

   return NULL;
}



/* save_bitmap:
 *  Writes a bitmap to disk.
 */
int save_bitmap(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   char tmp[32], *aext;
   BITMAP_TYPE_INFO *iter;
   ASSERT(filename);
   ASSERT(bmp);

   aext = uconvert_toascii(get_extension(filename), tmp);

   for (iter = bitmap_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->save)
	    return iter->save(filename, bmp, pal);
	 return 1;
      }
   }

   return 1;
}



/* _fixup_loaded_bitmap:
 *  Helper function for adjusting the color depth of a loaded image.
 *  Converts the bitmap BMP to the color depth BPP. If BMP is a 8-bit
 *  bitmap, PAL must be the palette attached to the bitmap. If BPP is
 *  equal to 8, the conversion is performed either by building a palette
 *  optimized for the bitmap if PAL is not NULL (in which case PAL gets
 *  filled in with this palette) or by using the current palette if PAL
 *  is NULL. In any other cases, PAL is unused.
 */
BITMAP *_fixup_loaded_bitmap(BITMAP *bmp, PALETTE pal, int bpp)
{
   BITMAP *b2;
   ASSERT(bmp);

   b2 = create_bitmap_ex(bpp, bmp->w, bmp->h);
   if (!b2) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (bpp == 8) {
      RGB_MAP *old_map = rgb_map;

      if (pal)
	 generate_optimized_palette(bmp, pal, NULL);
      else
	 pal = _current_palette;

      rgb_map = _AL_MALLOC(sizeof(RGB_MAP));
      if (rgb_map != NULL)
	 create_rgb_table(rgb_map, pal, NULL);

      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);

      if (rgb_map != NULL)
	 _AL_FREE(rgb_map);
      rgb_map = old_map;
   }
   else if (bitmap_color_depth(bmp) == 8) {
      select_palette(pal);
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
      unselect_palette();
   }
   else {
      blit(bmp, b2, 0, 0, 0, 0, bmp->w, bmp->h);
   }

   destroy_bitmap(bmp);

   return b2;
}



/* register_bitmap_file_type_exit:
 *  Free list of registered bitmap file types.
 */
static void register_bitmap_file_type_exit(void)
{
   BITMAP_TYPE_INFO *iter = bitmap_type_list, *next;

   while (iter) {
      next = iter->next;
      _AL_FREE(iter->ext);
      _AL_FREE(iter);
      iter = next;
   }
   
   bitmap_type_list = NULL;

   /* If we are using a destructor, then we only want to prune the list
    * down to valid modules. So we clean up as usual, but then reinstall
    * the internal modules.
    */
   #ifdef ALLEGRO_USE_CONSTRUCTOR
      _register_bitmap_file_type_init();
   #endif

   _remove_exit_func(register_bitmap_file_type_exit);
}



/* _register_bitmap_file_type_init:
 *  Register built-in bitmap file types.
 */
void _register_bitmap_file_type_init(void)
{
   char buf[32];

   _add_exit_func(register_bitmap_file_type_exit,
		  "register_bitmap_file_type_exit");

   register_bitmap_file_type(uconvert_ascii("bmp", buf), load_bmp, save_bmp);
   register_bitmap_file_type(uconvert_ascii("lbm", buf), load_lbm, NULL);
   register_bitmap_file_type(uconvert_ascii("pcx", buf), load_pcx, save_pcx);
   register_bitmap_file_type(uconvert_ascii("tga", buf), load_tga, save_tga);
}



#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(static void bitmap_filetype_constructor(void));
   DESTRUCTOR_FUNCTION(static void bitmap_filetype_destructor(void));

   /* bitmap_filetype_constructor:
    *  Register bitmap filetype functions if this object file is linked
    *  in. This isn't called if the load_bitmap() and save_bitmap()
    *  functions aren't used in a program, thus saving a little space
    *  in statically linked programs.
    */
   static void bitmap_filetype_constructor(void)
   {
      _register_bitmap_file_type_init();
   }

   /* bitmap_filetype_destructor:
    *  Since we only want to destroy the whole list when we *actually*
    *  quit, not just when allegro_exit() is called, we need to use a
    *  destructor to accomplish this.
    */
   static void bitmap_filetype_destructor(void)
   {
      BITMAP_TYPE_INFO *iter = bitmap_type_list, *next;

      while (iter) {
         next = iter->next;
         _AL_FREE(iter->ext);
         _AL_FREE(iter);
         iter = next;
      }
   
      bitmap_type_list = NULL;

      _remove_exit_func(register_bitmap_file_type_exit);
   }
#endif
