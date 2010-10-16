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
 *      List of DOS graphics drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



BEGIN_GFX_DRIVER_LIST
   GFX_DRIVER_VBEAF
   GFX_DRIVER_VGA
   GFX_DRIVER_MODEX
   GFX_DRIVER_VESA3
   GFX_DRIVER_VESA2L
   GFX_DRIVER_VESA2B
   GFX_DRIVER_XTENDED
   GFX_DRIVER_VESA1
END_GFX_DRIVER_LIST

