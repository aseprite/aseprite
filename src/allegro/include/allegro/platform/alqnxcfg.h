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
 *      Configuration defines for use with QNX.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <unistd.h>
#endif


/* a static auto config */
#define ALLEGRO_HAVE_INTTYPES_H 1
#define ALLEGRO_HAVE_STDINT_H   1
#define ALLEGRO_HAVE_STRICMP    1
#define ALLEGRO_HAVE_STRLWR     1
#define ALLEGRO_HAVE_STRUPR     1
#define ALLEGRO_HAVE_MEMCMP     1
#define ALLEGRO_HAVE_MKSTEMP    1
#define ALLEGRO_HAVE_DIRENT_H   1
#define ALLEGRO_HAVE_SYS_UTSNAME_H 1
#define ALLEGRO_HAVE_SYS_TIME_H 1
#define ALLEGRO_HAVE_LIBPTHREAD 1

/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "QNX"
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_HAVE_SCHED_YIELD
#define ALLEGRO_USE_CONSTRUCTOR
#undef ALLEGRO_MULTITHREADED  /* FIXME */

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/alqnx.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintqnx.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/qnx/asmcapa.h"
