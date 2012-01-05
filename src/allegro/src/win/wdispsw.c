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
 *      Display switch handling.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"


#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wdispsw INFO: "
#define PREFIX_W                "al-wdispsw WARNING: "
#define PREFIX_E                "al-wdispsw ERROR: "


int _win_app_foreground = TRUE;



/* _win_reset_switch_mode:
 *  Resets the switch mode to its default state.
 */
void _win_reset_switch_mode(void)
{
   /* The default state must be SWITCH_BACKGROUND so that the
      threads don't get blocked when the focus moves forth and
      back during window creation and destruction.  This seems
      to be particularly relevant to WinXP.  */
   set_display_switch_mode(SWITCH_BACKGROUND);

   _win_app_foreground = TRUE;
}



/* sys_directx_display_switch_init:
 */
void sys_directx_display_switch_init(void)
{
   _win_reset_switch_mode();
}



/* sys_directx_display_switch_exit:
 */
void sys_directx_display_switch_exit(void)
{
}



/* sys_directx_set_display_switch_mode:
 */
int sys_directx_set_display_switch_mode(int mode)
{
   switch (mode) {

      case SWITCH_BACKGROUND:
      case SWITCH_PAUSE:
         if (win_gfx_driver && !win_gfx_driver->has_backing_store)
            return -1;
         break;

      case SWITCH_BACKAMNESIA:
      case SWITCH_AMNESIA:
         if (!win_gfx_driver || win_gfx_driver->has_backing_store)
            return -1;
         break;

      default:
         return -1;
   }

   return 0;
}



/* _win_switch_in:
 *  Puts the library in the foreground.
 */
void _win_switch_in(void)
{
   _TRACE(PREFIX_I "switch in\n");

   _win_app_foreground = TRUE;

   /* update keyboard modifiers  */
   _al_win_kbd_update_shifts();

   if (win_gfx_driver && win_gfx_driver->switch_in)
      win_gfx_driver->switch_in();

   _switch_in();
}



/* _win_switch_out:
 *  Puts the library in the background.
 */
void _win_switch_out(void)
{
   _TRACE(PREFIX_I "switch out\n");

   _win_app_foreground = FALSE;

   if (win_gfx_driver && win_gfx_driver->switch_out)
      win_gfx_driver->switch_out();

   _switch_out();
}



/* _win_thread_switch_out:
 *  Handles a switch out event for the calling thread.
 *  Returns TRUE if the thread was blocked, FALSE otherwise.
 */
int _win_thread_switch_out(void)
{
   return FALSE;
}
