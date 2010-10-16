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
 *      Linux timer driver list.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"


/* list the available drivers */
_DRIVER_INFO _linux_timer_driver_list[] =
{
#ifdef ALLEGRO_HAVE_LIBPTHREAD
   {  TIMERDRV_UNIX_PTHREADS,  &timerdrv_unix_pthreads, TRUE  },
#else
   {  TIMERDRV_UNIX_SIGALRM,   &timerdrv_unix_sigalrm,  TRUE  },
#endif
   {  0,                       NULL,                    0     }
};

