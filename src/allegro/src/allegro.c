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
 *      Assorted globals and setup/cleanup routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



#define PREFIX_I                "al-main INFO: "
#define PREFIX_W                "al-main WARNING: "
#define PREFIX_E                "al-main ERROR: "

#ifdef ALLEGRO_AMIGA
   #define LOGFILE "RAM:allegro.log"
#else
   #define LOGFILE "allegro.log"
#endif


/* in case you want to report version numbers */
char allegro_id[] = "Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR;


/* error message for sound and gfx init routines */
char allegro_error[ALLEGRO_ERROR_SIZE] = EMPTY_STRING;


/* current system driver */
SYSTEM_DRIVER *system_driver = NULL;


/* error value, which will work even with DLL linkage */
int *allegro_errno = NULL;


/* flag for how many times we have been initialised */
int _allegro_count = 0;


/* flag to know whether we are being called by the exit mechanism */
int _allegro_in_exit = FALSE;


/* flag to decide whether to disable the screensaver */
int _screensaver_policy = FULLSCREEN_DISABLED;


/* the graphics driver currently in use */
GFX_DRIVER *gfx_driver = NULL;


/* abilities of the current graphics driver */
int gfx_capabilities = 0;


/* a bitmap structure for accessing the physical screen */
BITMAP *screen = NULL;


/* info about the current graphics drawing mode */
ALLEGRO_TLS int _drawing_mode = DRAW_MODE_SOLID;

ALLEGRO_TLS BITMAP *_drawing_pattern = NULL;

ALLEGRO_TLS int _drawing_x_anchor = 0;
ALLEGRO_TLS int _drawing_y_anchor = 0;

ALLEGRO_TLS unsigned int _drawing_x_mask = 0;
ALLEGRO_TLS unsigned int _drawing_y_mask = 0;


/* default palette structures */
PALETTE black_palette;
PALETTE _current_palette;

int _current_palette_changed = 0xFFFFFFFF;


PALETTE desktop_palette =
{
   { 63, 63, 63, 0 },   { 63, 0,  0,  0 },   { 0,  63, 0,  0 },   { 63, 63, 0,  0 },
   { 0,  0,  63, 0 },   { 63, 0,  63, 0 },   { 0,  63, 63, 0 },   { 16, 16, 16, 0 },
   { 31, 31, 31, 0 },   { 63, 31, 31, 0 },   { 31, 63, 31, 0 },   { 63, 63, 31, 0 },
   { 31, 31, 63, 0 },   { 63, 31, 63, 0 },   { 31, 63, 63, 0 },   { 0,  0,  0,  0 }
};


PALETTE default_palette =
{
   { 0,  0,  0,  0 },   { 0,  0,  42, 0 },   { 0,  42, 0,  0 },   { 0,  42, 42, 0 },
   { 42, 0,  0,  0 },   { 42, 0,  42, 0 },   { 42, 21, 0,  0 },   { 42, 42, 42, 0 },
   { 21, 21, 21, 0 },   { 21, 21, 63, 0 },   { 21, 63, 21, 0 },   { 21, 63, 63, 0 },
   { 63, 21, 21, 0 },   { 63, 21, 63, 0 },   { 63, 63, 21, 0 },   { 63, 63, 63, 0 },
   { 0,  0,  0,  0 },   { 5,  5,  5,  0 },   { 8,  8,  8,  0 },   { 11, 11, 11, 0 },
   { 14, 14, 14, 0 },   { 17, 17, 17, 0 },   { 20, 20, 20, 0 },   { 24, 24, 24, 0 },
   { 28, 28, 28, 0 },   { 32, 32, 32, 0 },   { 36, 36, 36, 0 },   { 40, 40, 40, 0 },
   { 45, 45, 45, 0 },   { 50, 50, 50, 0 },   { 56, 56, 56, 0 },   { 63, 63, 63, 0 },
   { 0,  0,  63, 0 },   { 16, 0,  63, 0 },   { 31, 0,  63, 0 },   { 47, 0,  63, 0 },
   { 63, 0,  63, 0 },   { 63, 0,  47, 0 },   { 63, 0,  31, 0 },   { 63, 0,  16, 0 },
   { 63, 0,  0,  0 },   { 63, 16, 0,  0 },   { 63, 31, 0,  0 },   { 63, 47, 0,  0 },
   { 63, 63, 0,  0 },   { 47, 63, 0,  0 },   { 31, 63, 0,  0 },   { 16, 63, 0,  0 },
   { 0,  63, 0,  0 },   { 0,  63, 16, 0 },   { 0,  63, 31, 0 },   { 0,  63, 47, 0 },
   { 0,  63, 63, 0 },   { 0,  47, 63, 0 },   { 0,  31, 63, 0 },   { 0,  16, 63, 0 },
   { 31, 31, 63, 0 },   { 39, 31, 63, 0 },   { 47, 31, 63, 0 },   { 55, 31, 63, 0 },
   { 63, 31, 63, 0 },   { 63, 31, 55, 0 },   { 63, 31, 47, 0 },   { 63, 31, 39, 0 },
   { 63, 31, 31, 0 },   { 63, 39, 31, 0 },   { 63, 47, 31, 0 },   { 63, 55, 31, 0 },
   { 63, 63, 31, 0 },   { 55, 63, 31, 0 },   { 47, 63, 31, 0 },   { 39, 63, 31, 0 },
   { 31, 63, 31, 0 },   { 31, 63, 39, 0 },   { 31, 63, 47, 0 },   { 31, 63, 55, 0 },
   { 31, 63, 63, 0 },   { 31, 55, 63, 0 },   { 31, 47, 63, 0 },   { 31, 39, 63, 0 },
   { 45, 45, 63, 0 },   { 49, 45, 63, 0 },   { 54, 45, 63, 0 },   { 58, 45, 63, 0 },
   { 63, 45, 63, 0 },   { 63, 45, 58, 0 },   { 63, 45, 54, 0 },   { 63, 45, 49, 0 },
   { 63, 45, 45, 0 },   { 63, 49, 45, 0 },   { 63, 54, 45, 0 },   { 63, 58, 45, 0 },
   { 63, 63, 45, 0 },   { 58, 63, 45, 0 },   { 54, 63, 45, 0 },   { 49, 63, 45, 0 },
   { 45, 63, 45, 0 },   { 45, 63, 49, 0 },   { 45, 63, 54, 0 },   { 45, 63, 58, 0 },
   { 45, 63, 63, 0 },   { 45, 58, 63, 0 },   { 45, 54, 63, 0 },   { 45, 49, 63, 0 },
   { 0,  0,  28, 0 },   { 7,  0,  28, 0 },   { 14, 0,  28, 0 },   { 21, 0,  28, 0 },
   { 28, 0,  28, 0 },   { 28, 0,  21, 0 },   { 28, 0,  14, 0 },   { 28, 0,  7,  0 },
   { 28, 0,  0,  0 },   { 28, 7,  0,  0 },   { 28, 14, 0,  0 },   { 28, 21, 0,  0 },
   { 28, 28, 0,  0 },   { 21, 28, 0,  0 },   { 14, 28, 0,  0 },   { 7,  28, 0,  0 },
   { 0,  28, 0,  0 },   { 0,  28, 7,  0 },   { 0,  28, 14, 0 },   { 0,  28, 21, 0 },
   { 0,  28, 28, 0 },   { 0,  21, 28, 0 },   { 0,  14, 28, 0 },   { 0,  7,  28, 0 },
   { 14, 14, 28, 0 },   { 17, 14, 28, 0 },   { 21, 14, 28, 0 },   { 24, 14, 28, 0 },
   { 28, 14, 28, 0 },   { 28, 14, 24, 0 },   { 28, 14, 21, 0 },   { 28, 14, 17, 0 },
   { 28, 14, 14, 0 },   { 28, 17, 14, 0 },   { 28, 21, 14, 0 },   { 28, 24, 14, 0 },
   { 28, 28, 14, 0 },   { 24, 28, 14, 0 },   { 21, 28, 14, 0 },   { 17, 28, 14, 0 },
   { 14, 28, 14, 0 },   { 14, 28, 17, 0 },   { 14, 28, 21, 0 },   { 14, 28, 24, 0 },
   { 14, 28, 28, 0 },   { 14, 24, 28, 0 },   { 14, 21, 28, 0 },   { 14, 17, 28, 0 },
   { 20, 20, 28, 0 },   { 22, 20, 28, 0 },   { 24, 20, 28, 0 },   { 26, 20, 28, 0 },
   { 28, 20, 28, 0 },   { 28, 20, 26, 0 },   { 28, 20, 24, 0 },   { 28, 20, 22, 0 },
   { 28, 20, 20, 0 },   { 28, 22, 20, 0 },   { 28, 24, 20, 0 },   { 28, 26, 20, 0 },
   { 28, 28, 20, 0 },   { 26, 28, 20, 0 },   { 24, 28, 20, 0 },   { 22, 28, 20, 0 },
   { 20, 28, 20, 0 },   { 20, 28, 22, 0 },   { 20, 28, 24, 0 },   { 20, 28, 26, 0 },
   { 20, 28, 28, 0 },   { 20, 26, 28, 0 },   { 20, 24, 28, 0 },   { 20, 22, 28, 0 },
   { 0,  0,  16, 0 },   { 4,  0,  16, 0 },   { 8,  0,  16, 0 },   { 12, 0,  16, 0 },
   { 16, 0,  16, 0 },   { 16, 0,  12, 0 },   { 16, 0,  8,  0 },   { 16, 0,  4,  0 },
   { 16, 0,  0,  0 },   { 16, 4,  0,  0 },   { 16, 8,  0,  0 },   { 16, 12, 0,  0 },
   { 16, 16, 0,  0 },   { 12, 16, 0,  0 },   { 8,  16, 0,  0 },   { 4,  16, 0,  0 },
   { 0,  16, 0,  0 },   { 0,  16, 4,  0 },   { 0,  16, 8,  0 },   { 0,  16, 12, 0 },
   { 0,  16, 16, 0 },   { 0,  12, 16, 0 },   { 0,  8,  16, 0 },   { 0,  4,  16, 0 },
   { 8,  8,  16, 0 },   { 10, 8,  16, 0 },   { 12, 8,  16, 0 },   { 14, 8,  16, 0 },
   { 16, 8,  16, 0 },   { 16, 8,  14, 0 },   { 16, 8,  12, 0 },   { 16, 8,  10, 0 },
   { 16, 8,  8,  0 },   { 16, 10, 8,  0 },   { 16, 12, 8,  0 },   { 16, 14, 8,  0 },
   { 16, 16, 8,  0 },   { 14, 16, 8,  0 },   { 12, 16, 8,  0 },   { 10, 16, 8,  0 },
   { 8,  16, 8,  0 },   { 8,  16, 10, 0 },   { 8,  16, 12, 0 },   { 8,  16, 14, 0 },
   { 8,  16, 16, 0 },   { 8,  14, 16, 0 },   { 8,  12, 16, 0 },   { 8,  10, 16, 0 },
   { 11, 11, 16, 0 },   { 12, 11, 16, 0 },   { 13, 11, 16, 0 },   { 15, 11, 16, 0 },
   { 16, 11, 16, 0 },   { 16, 11, 15, 0 },   { 16, 11, 13, 0 },   { 16, 11, 12, 0 },
   { 16, 11, 11, 0 },   { 16, 12, 11, 0 },   { 16, 13, 11, 0 },   { 16, 15, 11, 0 },
   { 16, 16, 11, 0 },   { 15, 16, 11, 0 },   { 13, 16, 11, 0 },   { 12, 16, 11, 0 },
   { 11, 16, 11, 0 },   { 11, 16, 12, 0 },   { 11, 16, 13, 0 },   { 11, 16, 15, 0 },
   { 11, 16, 16, 0 },   { 11, 15, 16, 0 },   { 11, 13, 16, 0 },   { 11, 12, 16, 0 },
   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },
   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 0,  0,  0,  0 },   { 63, 63, 63, 0 }
};


/* colors for the standard GUI dialogs (alerts, file selector, etc) */
int gui_fg_color = 255;
int gui_mg_color = 8;
int gui_bg_color = 0;


/* a block of temporary working memory */
void *_scratch_mem = NULL;
int _scratch_mem_size = 0;


/* SVGA bank switching tables */
int _last_bank_1 = -1;
int _last_bank_2 = -1;
int *_gfx_bank = NULL;


/* what OS are we running under? */
int os_type = OSTYPE_UNKNOWN;
int os_version = -1;
int os_revision = -1;
int os_multitasking = FALSE;


/* processor information */
char cpu_vendor[_AL_CPU_VENDOR_SIZE] = EMPTY_STRING;
int cpu_family = 0;
int cpu_model = 0;
int cpu_capabilities = 0;


/* debugging stuff */
static int debug_assert_virgin = TRUE;
static int debug_trace_virgin = TRUE;

static FILE *assert_file = NULL;
static FILE *trace_file = NULL;

static int (*assert_handler)(AL_CONST char *msg) = NULL;
int (*_al_trace_handler)(AL_CONST char *msg) = NULL;


/* Module linking system stuff: if an object file is linked in, then its
 * constructor function is executed; this function should fill in the
 * data structures below (declared in aintern.h). If the module is not
 * linked in, then the structure pointers will be null, so we don't need
 * to bother with that bit of code elsewhere.
 */
struct _AL_LINKER_MOUSE *_al_linker_mouse = NULL;


/* dynamic registration system for cleanup code */
struct al_exit_func {
   void (*funcptr)(void);
   AL_CONST char *desc;
   struct al_exit_func *next;
};

static struct al_exit_func *exit_func_list = NULL;



/* _add_exit_func:
 *  Adds a function to the list that need to be called by allegro_exit().
 *  `desc' should point to a statically allocated string to help with
 *  debugging.
 */
void _add_exit_func(void (*func)(void), AL_CONST char *desc)
{
   struct al_exit_func *n;

   for (n = exit_func_list; n; n = n->next)
      if (n->funcptr == func)
         return;

   n = _AL_MALLOC(sizeof(struct al_exit_func));
   if (!n)
      return;

   n->next = exit_func_list;
   n->funcptr = func;
   n->desc = desc;
   exit_func_list = n;
}



/* _remove_exit_func:
 *  Removes a function from the list that need to be called by allegro_exit().
 */
void _remove_exit_func(void (*func)(void))
{
   struct al_exit_func *iter = exit_func_list, *prev = NULL;

   while (iter) {
      if (iter->funcptr == func) {
         if (prev)
            prev->next = iter->next;
         else
            exit_func_list = iter->next;
         _AL_FREE(iter);
         return;
      }
      prev = iter;
      iter = iter->next;
   }
}



/* allegro_exit_stub:
 *  Stub function registered by the library via atexit.
 */
static void allegro_exit_stub(void)
{
   _allegro_in_exit = TRUE;

   allegro_exit();
}



/* _install_allegro:
 *  Initialises the Allegro library, activating the system driver.
 */
static int _install_allegro(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void)))
{
   RGB black_rgb = {0, 0, 0, 0};
   char tmp1[64], tmp2[64];
   int i;

   #ifndef ALLEGRO_USE_CONSTRUCTOR
      /* call constructor functions manually */
      extern void _initialize_datafile_types();
      extern void _mouse_constructor();

      _initialize_datafile_types();
      _mouse_constructor();
      _register_bitmap_file_type_init();
      _register_font_file_type_init();
   #endif

   if (errno_ptr)
      allegro_errno = errno_ptr;
   else
      allegro_errno = &errno;

   /* set up default palette structures */
   for (i=0; i<256; i++)
      black_palette[i] = black_rgb;

   for (i=16; i<256; i++)
      desktop_palette[i] = desktop_palette[i & 15];

   /* lock some important variables */
   LOCK_VARIABLE(screen);
   LOCK_VARIABLE(gfx_driver);
   LOCK_VARIABLE(gfx_capabilities);
   LOCK_VARIABLE(_last_bank_1);
   LOCK_VARIABLE(_last_bank_2);
   LOCK_VARIABLE(_gfx_bank);
   LOCK_VARIABLE(_drawing_mode);
   LOCK_VARIABLE(_drawing_pattern);
   LOCK_VARIABLE(_drawing_x_anchor);
   LOCK_VARIABLE(_drawing_y_anchor);
   LOCK_VARIABLE(_drawing_x_mask);
   LOCK_VARIABLE(_drawing_y_mask);
   LOCK_VARIABLE(_current_palette);
   LOCK_VARIABLE(os_type);

   /* nasty stuff to set up the config system before the system driver */
   system_driver = _system_driver_list[0].driver;

   /* needed in case set_config_file was called before allegro_init */
   _reload_config();

   reload_config_texts(NULL);

   if (system_id == SYSTEM_AUTODETECT)
      system_id = get_config_id(uconvert_ascii("system", tmp1), uconvert_ascii("system", tmp2), SYSTEM_AUTODETECT);

   system_driver = NULL;

   /* initialise the system driver */
   usetc(allegro_error, 0);

   for (i=0; _system_driver_list[i].driver; i++) {
      if ((_system_driver_list[i].id == system_id) ||
          ((_system_driver_list[i].autodetect) && (system_id == SYSTEM_AUTODETECT))) {
         system_driver = _system_driver_list[i].driver;
         system_driver->name = system_driver->desc = get_config_text(system_driver->ascii_name);
         if (system_driver->init() != 0) {
            system_driver = NULL;
            if (system_id != SYSTEM_AUTODETECT)
               break;
         }
         else
            break;
      }
   }

   if (!system_driver)
      return -1;

   /* disable close button */
   set_close_button_callback(NULL);

   /* disable resize */
   set_resize_callback(NULL);

   /* detect CPU type */
   check_cpu();

#if defined(ALLEGRO_UNIX) || defined(ALLEGRO_WINDOWS)
   /* detect filename encoding used by libc */
   /* XXX This should be done for all platforms but I'm not set up to check
    * that all platforms have been covered before making the 4.2.3 release.
    * --pw
    */
   _al_detect_filename_encoding();
#endif

   /* set up screensaver policy */
   _screensaver_policy = get_config_int(uconvert_ascii("system", tmp1),
                                        uconvert_ascii("disable_screensaver", tmp2),
                                        FULLSCREEN_DISABLED);

   /* install shutdown handler */
   if (_allegro_count == 0) {
      if (atexit_ptr)
         atexit_ptr(allegro_exit_stub);
   }

   _allegro_count++;

   TRACE(PREFIX_I "Allegro initialised (instance %d)\n", _allegro_count);
   return 0;
}



/* _install_allegro_version_check:
 *  Initialises the Allegro library, but return with an error if an
 *  incompatible version is found.
 */
int _install_allegro_version_check(int system_id, int *errno_ptr,
   int (*atexit_ptr)(void (*func)(void)), int version)
{
   int r = _install_allegro(system_id, errno_ptr, atexit_ptr);

   int build_wip = version & 255;
   int build_ver = version & ~255;

   int version_ok;

   if (r != 0) {
      /* failed */
      return r;
   }

#if ALLEGRO_SUB_VERSION & 1
   /* This is a WIP runtime, so enforce strict compatibility. */
   version_ok = version == MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION);
#else
   /* This is a stable runtime, so the runtime should be at least as new
    * as the build headers (otherwise we may get a crash, since some
    * functions may have been used which aren't available in this runtime).
    */
   version_ok = (MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, 0) == build_ver) &&
      (ALLEGRO_WIP_VERSION >= build_wip);
#endif

   if (!version_ok) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(
         "The detected dynamic Allegro library (%d.%d.%d) is "
         "not compatible with this program (%d.%d.%d)."),
         ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION,
         build_ver >> 16, (build_ver >> 8) & 255, build_wip);
      return -1;
   }
   return 0;
}



/* allegro_exit:
 *  Closes down the Allegro system.
 */
void allegro_exit(void)
{
   while (exit_func_list) {
      void (*func)(void) = exit_func_list->funcptr;
      _remove_exit_func(func);
      (*func)();
   }

   if (system_driver) {
      system_driver->exit();
      system_driver = NULL;
   }

   if (_scratch_mem) {
      _AL_FREE(_scratch_mem);
      _scratch_mem = NULL;
      _scratch_mem_size = 0;
   }
}



/* allegro_message:
 *  Displays a message in whatever form the current platform requires.
 */
void allegro_message(AL_CONST char *msg, ...)
{
   char *buf = _AL_MALLOC_ATOMIC(ALLEGRO_MESSAGE_SIZE);
   char *tmp = _AL_MALLOC_ATOMIC(ALLEGRO_MESSAGE_SIZE);
   va_list ap;
   ASSERT(msg);
   va_start(ap, msg);
   uvszprintf(buf, ALLEGRO_MESSAGE_SIZE, msg, ap);
   va_end(ap);

   if ((system_driver) && (system_driver->message))
      system_driver->message(buf);
   else
      fputs(uconvert(buf, U_CURRENT, tmp, U_ASCII_CP, ALLEGRO_MESSAGE_SIZE), stdout);

   _AL_FREE(buf);
   _AL_FREE(tmp);
}



/* get_executable_name:
 *  Finds out the name of the current executable.
 */
void get_executable_name(char *output, int size)
{
   ASSERT(system_driver);
   ASSERT(output);

   if (system_driver->get_executable_name) {
      system_driver->get_executable_name(output, size);
   }
   else {
      output += usetc(output, '.');
      output += usetc(output, '/');
      usetc(output, 0);
   }
}



/* set_close_button_callback:
 *  Installs a callback function to be called when the close button
 *  of the window or any equivalent device is activated. Returns 0 on
 *  success and -1 on failure (e.g. feature not supported).
 */
int set_close_button_callback(void (*proc)(void))
{
   ASSERT(system_driver);

   if (system_driver->set_close_button_callback)
      return system_driver->set_close_button_callback(proc);

   return -1;
}



/* set_resize_callback:
 *  Installs a callback function to be called when the window is
 *  resized.
 */
int set_resize_callback(void (*proc)(RESIZE_DISPLAY_EVENT *ev))
{
   ASSERT(system_driver);

   if (system_driver->set_resize_callback)
      return system_driver->set_resize_callback(proc);

   return -1;
}



/* debug_exit:
 *  Closes the debugging output files.
 */
static void debug_exit(void)
{
   if (assert_file) {
      fclose(assert_file);
      assert_file = NULL;
   }

   if (trace_file) {
      fclose(trace_file);
      trace_file = NULL;
   }

   debug_assert_virgin = TRUE;
   debug_trace_virgin = TRUE;

   _remove_exit_func(debug_exit);
}



/* al_assert:
 *  Raises an assert (uses ASCII strings).
 */
void al_assert(AL_CONST char *file, int line)
{
   static int asserted = FALSE;
   int olderr = errno;
   char buf[128];
   char *s;

   if (asserted)
      return;

   /* todo, some day: use snprintf (C99) */
   sprintf(buf, "Assert failed at line %d of %s", line, file);

   if (assert_handler) {
      if (assert_handler(buf))
         return;
   }

   if (debug_assert_virgin) {
      s = getenv("ALLEGRO_ASSERT");

      if (s)
         assert_file = fopen(s, "w");
      else
         assert_file = NULL;

      if (debug_trace_virgin)
         _add_exit_func(debug_exit, "debug_exit");

      debug_assert_virgin = FALSE;
   }

   if (assert_file) {
      fprintf(assert_file, "%s\n", buf);
      fflush(assert_file);
   }
   else {
      asserted = TRUE;

      if ((system_driver) && (system_driver->assert)) {
         system_driver->assert(buf);
      }
      else {
         allegro_exit();
         fprintf(stderr, "%s\n", buf);
         abort();
      }
   }

   errno = olderr;
}



/* al_trace:
 *  Outputs a trace message (uses ASCII strings).
 */
void al_trace(AL_CONST char *msg, ...)
{
   int olderr = errno;
   char buf[512];
   char *s;

   /* todo, some day: use vsnprintf (C99) */
   va_list ap;
   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   if (_al_trace_handler) {
      if (_al_trace_handler(buf))
         return;
   }

   if (debug_trace_virgin) {
      s = getenv("ALLEGRO_TRACE");

      if (s)
         trace_file = fopen(s, "w");
      else
         trace_file = fopen(LOGFILE, "w");

      if (debug_assert_virgin)
         _add_exit_func(debug_exit, "debug_exit");

      debug_trace_virgin = FALSE;
   }

   if (trace_file) {
      fwrite(buf, sizeof(char), strlen(buf), trace_file);
      fflush(trace_file);
   }

   errno = olderr;
}



/* register_assert_handler:
 *  Installs a user handler for assert failures.
 */
void register_assert_handler(int (*handler)(AL_CONST char *msg))
{
   assert_handler = handler;
}



/* register_trace_handler:
 *  Installs a user handler for trace output.
 */
void register_trace_handler(int (*handler)(AL_CONST char *msg))
{
   _al_trace_handler = handler;
}



/* _al_malloc:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void *_al_malloc(size_t size)
{
   return malloc(size);
}



/* _al_free:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void _al_free(void *mem)
{
   free(mem);
}



/* _al_realloc:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void *_al_realloc(void *mem, size_t size)
{
   return realloc(mem, size);
}



/* _al_strdup:
 *  Wrapper for when a program needs to duplicate a string in a way that
 *  uses any user overloaded memory allocation system in use.
 *  The result of this function _must_ be freed with _AL_FREE().
 */
char *_al_strdup(AL_CONST char *string)
{
   char *newstring = _AL_MALLOC(strlen(string) + 1);

   if (newstring)
      strcpy(newstring, string);

   return newstring;
}



/* a simple system driver for platform-independent code */
static int sys_none_init(void) { return 0; }
static void sys_none_exit(void) { }

static _DRIVER_INFO no_drivers[] = {{ 0, NULL, 0 }};
static _DRIVER_INFO *sys_no_driver(void) { return no_drivers; }


SYSTEM_DRIVER system_none =
{
   SYSTEM_NONE,
   empty_string,
   empty_string,
   "Agnostic",
   sys_none_init,
   sys_none_exit,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   sys_no_driver, sys_no_driver, sys_no_driver, sys_no_driver
};
