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
 *      System inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_SYSTEM_INL
#define ALLEGRO_SYSTEM_INL

#include "allegro/debug.h"

#ifdef __cplusplus
   extern "C" {
#endif


AL_INLINE(void, set_window_title, (AL_CONST char *name),
{
   ASSERT(system_driver);

   if (system_driver->set_window_title)
      system_driver->set_window_title(name);
})


AL_INLINE(int, desktop_color_depth, (void),
{
   ASSERT(system_driver);

   if (system_driver->desktop_color_depth)
      return system_driver->desktop_color_depth();
   else
      return 0;
})


AL_INLINE(int, get_desktop_resolution, (int *width, int *height),
{
   ASSERT(system_driver);

   if (system_driver->get_desktop_resolution)
      return system_driver->get_desktop_resolution(width, height);
   else
      return -1;
})



#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_SYSTEM_INL */


