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
 *      Configuration defines for use with BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */


#include <fcntl.h>
#include <unistd.h>

/* provide implementations of missing functions */
#define ALLEGRO_NO_STRICMP
#define ALLEGRO_NO_STRLWR
#define ALLEGRO_NO_STRUPR

/* a static auto config */
#define ALLEGRO_HAVE_DIRENT_H   1
#define ALLEGRO_HAVE_INTTYPES_H 1       /* TODO: check this */
#define ALLEGRO_HAVE_STDINT_H   1       /* TODO: check this */
#define ALLEGRO_HAVE_SYS_TIME_H 1

/* describe this platform */
#if defined __BEOS__ && !defined __HAIKU__
  #define ALLEGRO_PLATFORM_STR  "BeOS"
#endif
#if defined __HAIKU__
  #define ALLEGRO_PLATFORM_STR  "Haiku"
  #define ALLEGRO_HAVE_LIBPTHREAD 1
#endif
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_USE_CONSTRUCTOR
#define ALLEGRO_MULTITHREADED

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/albeos.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintbeos.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/beos/asmcapa.h"
