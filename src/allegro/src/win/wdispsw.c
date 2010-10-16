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

static HANDLE foreground_event = NULL;
static int allegro_thread_priority = THREAD_PRIORITY_NORMAL;



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
   
   /* This has a nice side-effect: releasing the blocked threads. */
   SetEvent(foreground_event);
}



/* sys_directx_display_switch_init:
 */
void sys_directx_display_switch_init(void)
{
   foreground_event = CreateEvent(NULL, TRUE, TRUE, NULL);
   _win_reset_switch_mode();
}



/* sys_directx_display_switch_exit:
 */
void sys_directx_display_switch_exit(void)
{
   CloseHandle(foreground_event);
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
   int mode;

   _TRACE(PREFIX_I "switch in\n");

   _win_app_foreground = TRUE;

   key_dinput_acquire();
   mouse_dinput_acquire();
   joystick_dinput_acquire();

   if (win_gfx_driver && win_gfx_driver->switch_in)
      win_gfx_driver->switch_in();

   /* handle switch modes */
   mode = get_display_switch_mode();

   if ((mode == SWITCH_AMNESIA) || (mode == SWITCH_PAUSE)) {
      _TRACE(PREFIX_I "AMNESIA or PAUSE mode recovery\n");
      SetEvent(foreground_event);

      /* restore old priority and wake up */
      SetThreadPriority(allegro_thread, allegro_thread_priority);
   }

   _switch_in();
}



/* _win_switch_out:
 *  Puts the library in the background.
 */
void _win_switch_out(void)
{
   int mode;

   _TRACE(PREFIX_I "switch out\n");

   _win_app_foreground = FALSE;

   key_dinput_unacquire();
   mouse_dinput_unacquire();
   joystick_dinput_unacquire();

   midi_switch_out();

   if (win_gfx_driver && win_gfx_driver->switch_out)
      win_gfx_driver->switch_out();

   /* handle switch modes */
   mode = get_display_switch_mode();

   if ((mode == SWITCH_AMNESIA) || (mode == SWITCH_PAUSE)) {
      _TRACE(PREFIX_I "AMNESIA or PAUSE mode suspension\n");
      ResetEvent(foreground_event);

      /* if the thread doesn't stop, lower its priority only if another window is active */ 
      allegro_thread_priority = GetThreadPriority(allegro_thread); 
      if ((HINSTANCE)GetWindowLong(GetForegroundWindow(), GWL_HINSTANCE) != allegro_inst)
	 SetThreadPriority(allegro_thread, THREAD_PRIORITY_LOWEST); 
   }

   _switch_out();
}



/* _win_thread_switch_out:
 *  Handles a switch out event for the calling thread.
 *  Returns TRUE if the thread was blocked, FALSE otherwise.
 */
int _win_thread_switch_out(void)
{
   int mode = get_display_switch_mode();

   if ((mode == SWITCH_AMNESIA) || (mode == SWITCH_PAUSE)) {
      WaitForSingleObject(foreground_event, INFINITE);
      return TRUE;
   }

   return FALSE;
}
