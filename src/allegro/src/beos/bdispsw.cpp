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
 *      BeOS display switching routines.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif

int _be_switch_mode = SWITCH_PAUSE;



/* be_sys_set_display_switch_mode:
 *  Initializes new display switching mode.
 */
extern "C" int be_sys_set_display_switch_mode(int mode)
{  
   /* Fullscreen only supports SWITCH_AMNESIA */
   if ((_be_allegro_screen) && (mode != SWITCH_AMNESIA))
      return -1;

   _be_switch_mode = mode;

   return 0;
}
