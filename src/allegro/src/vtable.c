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
 *      List of available bitmap vtables, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"


#ifndef ALLEGRO_COLOR8
#undef COLOR_DEPTH_8
#define COLOR_DEPTH_8
#endif


#ifndef ALLEGRO_COLOR16
#undef COLOR_DEPTH_15
#undef COLOR_DEPTH_16
#define COLOR_DEPTH_15
#define COLOR_DEPTH_16
#endif


#ifndef ALLEGRO_COLOR24
#undef COLOR_DEPTH_24
#define COLOR_DEPTH_24
#endif


#ifndef ALLEGRO_COLOR32
#undef COLOR_DEPTH_32
#define COLOR_DEPTH_32
#endif


BEGIN_COLOR_DEPTH_LIST
   COLOR_DEPTH_8
   COLOR_DEPTH_15
   COLOR_DEPTH_16
   COLOR_DEPTH_24
   COLOR_DEPTH_32
END_COLOR_DEPTH_LIST


