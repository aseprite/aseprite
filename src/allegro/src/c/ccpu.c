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
 *      Dummy CPU detection routines.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

/* MacOS X has its own check_cpu function, see src/macosx/pcpu.m */
#ifndef ALLEGRO_MACOSX

/* check_cpu:
 *  This is the function to call to set the globals.
 */
void check_cpu(void)
{
   cpu_family = 0;
   cpu_model = 0;
   cpu_capabilities = 0;
}

#endif
