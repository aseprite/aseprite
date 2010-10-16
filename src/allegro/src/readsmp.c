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
 *      Top level sample type registration routines.
 *
 *      By Vincent Penquerc'h.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct SAMPLE_TYPE_INFO
{
   char *ext;
   SAMPLE *(*load)(AL_CONST char *filename);
   int (*save)(AL_CONST char *filename, SAMPLE *smp);
   struct SAMPLE_TYPE_INFO *next;
} SAMPLE_TYPE_INFO;

static SAMPLE_TYPE_INFO *sample_type_list = NULL;



/* register_sample_file_type:
 *  Informs Allegro of a new sample file type, telling it how to load and
 *  save files of this format (either function may be NULL).
 */
void register_sample_file_type(AL_CONST char *ext, SAMPLE *(*load)(AL_CONST char *filename), int (*save)(AL_CONST char *filename, SAMPLE *smp))
{
   char tmp[32], *aext;
   SAMPLE_TYPE_INFO *iter = sample_type_list;
   ASSERT(ext);

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) 
      iter = sample_type_list = _AL_MALLOC(sizeof(struct SAMPLE_TYPE_INFO));
   else {
      for (iter = sample_type_list; iter->next; iter = iter->next);
      iter = iter->next = _AL_MALLOC(sizeof(struct SAMPLE_TYPE_INFO));
   }

   if (iter) {
      iter->load = load;
      iter->save = save;
      iter->ext = _al_strdup(aext);
      iter->next = NULL;
   }
}



/* load_sample:
 *  Loads a sample from disk.
 */
SAMPLE *load_sample(AL_CONST char *filename)
{
   char tmp[32], *aext;
   SAMPLE_TYPE_INFO *iter;
   ASSERT(filename);

   aext = uconvert_toascii(get_extension(filename), tmp);
   
   for (iter = sample_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->load)
	    return iter->load(filename);
	 return NULL;
      }
   }

   return NULL;
}



/* save_sample:
 *  Writes a sample to disk.
 */
int save_sample(AL_CONST char *filename, SAMPLE *smp)
{
   char tmp[32], *aext;
   SAMPLE_TYPE_INFO *iter;
   ASSERT(filename);
   ASSERT(smp);

   aext = uconvert_toascii(get_extension(filename), tmp);

   for (iter = sample_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->save)
	    return iter->save(filename, smp);
	 return 1;
      }
   }

   return 1;
}



/* register_sample_file_type_exit:
 *  Free list of registered sample file types.
 */
static void register_sample_file_type_exit(void)
{
   SAMPLE_TYPE_INFO *iter = sample_type_list, *next;

   while (iter) {
      next = iter->next;
      _AL_FREE(iter->ext);
      _AL_FREE(iter);
      iter = next;
   }
   
   sample_type_list = NULL;

   /* If we are using a destructor, then we only want to prune the list
    * down to valid modules. So we clean up as usual, but then reinstall
    * the internal modules.
    */
   #ifdef ALLEGRO_USE_CONSTRUCTOR
      _register_sample_file_type_init();
   #endif

   _remove_exit_func(register_sample_file_type_exit);
}



/* _register_sample_file_type_init:
 *  Register built-in sample file types.
 */
void _register_sample_file_type_init(void)
{
   char buf[32];

   _add_exit_func(register_sample_file_type_exit,
		  "register_sample_file_type_exit");

   register_sample_file_type(uconvert_ascii("wav", buf), load_wav, NULL);
   register_sample_file_type(uconvert_ascii("voc", buf), load_voc, NULL);
}



#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(static void sample_filetype_constructor(void));
   DESTRUCTOR_FUNCTION(static void sample_filetype_destructor(void));

   /* sample_filetype_constructor:
    *  Register sample filetype functions if this object file is linked
    *  in. This isn't called if the load_sample() and save_sample()
    *  functions aren't used in a program, thus saving a little space
    *  in statically linked programs.
    */
   static void sample_filetype_constructor(void)
   {
      _register_sample_file_type_init();
   }

   /* sample_filetype_destructor:
    *  Since we only want to destroy the whole list when we *actually*
    *  quit, not just when allegro_exit() is called, we need to use a
    *  destructor to accomplish this.
    */
   static void sample_filetype_destructor(void)
   {
      SAMPLE_TYPE_INFO *iter = sample_type_list, *next;

      while (iter) {
         next = iter->next;
         _AL_FREE(iter->ext);
         _AL_FREE(iter);
         iter = next;
      }
   
      sample_type_list = NULL;

      _remove_exit_func(register_sample_file_type_exit);
   }
#endif
