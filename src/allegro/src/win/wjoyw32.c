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
 *      Win32 joystick driver.
 *
 *      By Stefan Schimanski.
 *
 *      Bugfixes by Jose Antonio Luque.
 *
 *      Bugfixes and enhanced POV support by Johan Peitz.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wjoy INFO: "
#define PREFIX_W                "al-wjoy WARNING: "
#define PREFIX_E                "al-wjoy ERROR: "


static int joystick_win32_init(void);
static void joystick_win32_exit(void);
static int joystick_win32_poll(void);


JOYSTICK_DRIVER joystick_win32 =
{
   JOY_TYPE_WIN32,
   empty_string,
   empty_string,
   "Win32 joystick",
   joystick_win32_init,
   joystick_win32_exit,
   joystick_win32_poll,
   NULL,
   NULL,
   NULL,
   NULL
};


struct WIN32_JOYSTICK_INFO {
   WINDOWS_JOYSTICK_INFO_MEMBERS
   int device;
   int axis_min[WINDOWS_MAX_AXES];
   int axis_max[WINDOWS_MAX_AXES];
};

struct WIN32_JOYSTICK_INFO win32_joystick[MAX_JOYSTICKS];
static int win32_joy_num = 0;



/* joystick_win32_poll:
 *  Polls the Win32 joystick devices.
 */
static int joystick_win32_poll(void)
{
   int n_joy, n_axis, n_but, p, range;
   JOYINFOEX js;

   for (n_joy = 0; n_joy < win32_joy_num; n_joy++) {
      js.dwSize = sizeof(js);
      js.dwFlags = JOY_RETURNALL;

      if (joyGetPosEx(win32_joystick[n_joy].device, &js) == JOYERR_NOERROR) {

	 /* axes */
	 win32_joystick[n_joy].axis[0] = js.dwXpos;
	 win32_joystick[n_joy].axis[1] = js.dwYpos;
	 n_axis = 2;

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASZ) {
            win32_joystick[n_joy].axis[n_axis] = js.dwZpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASR) {
            win32_joystick[n_joy].axis[n_axis] = js.dwRpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASU) {
            win32_joystick[n_joy].axis[n_axis] = js.dwUpos;
            n_axis++;
         }

	 if (win32_joystick[n_joy].caps & JOYCAPS_HASV) {
            win32_joystick[n_joy].axis[n_axis] = js.dwVpos;
            n_axis++;
         }

         /* map Windows axis range to 0-256 Allegro range */
         for (n_axis = 0; n_axis < win32_joystick[n_joy].num_axes; n_axis++) {
            p = win32_joystick[n_joy].axis[n_axis] - win32_joystick[n_joy].axis_min[n_axis];
            range = win32_joystick[n_joy].axis_max[n_axis] - win32_joystick[n_joy].axis_min[n_axis];

            if (range > 0)
               win32_joystick[n_joy].axis[n_axis] = p * 256 / range;
            else
               win32_joystick[n_joy].axis[n_axis] = 0;
         }

	 /* hat */
         if (win32_joystick[n_joy].caps & JOYCAPS_HASPOV)
            win32_joystick[n_joy].hat = js.dwPOV;

	 /* buttons */
	 for (n_but = 0; n_but < win32_joystick[n_joy].num_buttons; n_but++)
	    win32_joystick[n_joy].button[n_but] = ((js.dwButtons & (1 << n_but)) != 0);
      }
      else {
         for(n_axis = 0; n_axis<win32_joystick[n_joy].num_axes; n_axis++) 
            win32_joystick[n_joy].axis[n_axis] = 0;

         if (win32_joystick[n_joy].caps & JOYCAPS_HASPOV)
            win32_joystick[n_joy].hat = 0;

	 for (n_but = 0; n_but < win32_joystick[n_joy].num_buttons; n_but++)
	    win32_joystick[n_joy].button[n_but] = FALSE;
      }

      win_update_joystick_status(n_joy, (WINDOWS_JOYSTICK_INFO *)&win32_joystick[n_joy]);
   }

   return 0;
}



/* joystick_win32_init:
 *  Initialises the Win32 joystick driver.
 */
static int joystick_win32_init(void)
{
   JOYCAPS caps;
   JOYINFOEX js;
   int n_joyat, n_joy, n_axis;

   win32_joy_num = joyGetNumDevs();

   if (win32_joy_num > MAX_JOYSTICKS)
      _TRACE(PREFIX_W "The system supports more than %d joysticks\n", MAX_JOYSTICKS);

   /* retrieve joystick infos */
   n_joy = 0;
   for (n_joyat = 0; n_joyat < win32_joy_num; n_joyat++) {
      if (n_joy == MAX_JOYSTICKS)
         break;

      if (joyGetDevCaps(n_joyat, &caps, sizeof(caps)) == JOYERR_NOERROR) {
         /* is the joystick physically attached? */
         js.dwSize = sizeof(js);
         js.dwFlags = JOY_RETURNALL;
         if (joyGetPosEx(n_joyat, &js) == JOYERR_UNPLUGGED)
            continue;

         memset(&win32_joystick[n_joy], 0, sizeof(struct WIN32_JOYSTICK_INFO));

         /* set global properties */
	 win32_joystick[n_joy].device = n_joyat;
	 win32_joystick[n_joy].caps = caps.wCaps;
	 win32_joystick[n_joy].num_buttons = MIN(caps.wNumButtons, MAX_JOYSTICK_BUTTONS);
	 win32_joystick[n_joy].num_axes = MIN(caps.wNumAxes, WINDOWS_MAX_AXES);

	 /* fill in ranges of axes */
	 win32_joystick[n_joy].axis_min[0] = caps.wXmin;
	 win32_joystick[n_joy].axis_max[0] = caps.wXmax;
	 win32_joystick[n_joy].axis_min[1] = caps.wYmin;
	 win32_joystick[n_joy].axis_max[1] = caps.wYmax;
	 n_axis = 2;

	 if (caps.wCaps & JOYCAPS_HASZ)	{
	    win32_joystick[n_joy].axis_min[2] = caps.wZmin;
	    win32_joystick[n_joy].axis_max[2] = caps.wZmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASR)	{
	    win32_joystick[n_joy].axis_min[n_axis] = caps.wRmin;
	    win32_joystick[n_joy].axis_max[n_axis] = caps.wRmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASU)	{
	    win32_joystick[n_joy].axis_min[n_axis] = caps.wUmin;
	    win32_joystick[n_joy].axis_max[n_axis] = caps.wUmax;
	    n_axis++;
	 }

	 if (caps.wCaps & JOYCAPS_HASV)	{
	    win32_joystick[n_joy].axis_min[n_axis] = caps.wVmin;
	    win32_joystick[n_joy].axis_max[n_axis] = caps.wVmax;
	    n_axis++;
	 }

         /* register this joystick */
         if (win_add_joystick((WINDOWS_JOYSTICK_INFO *)&win32_joystick[n_joy]) != 0)
            break;

         n_joy++;
      }
   }

   win32_joy_num = n_joy;

   return (win32_joy_num == 0);
}



/* joystick_win32_exit:
 *  Shuts down the Win32 joystick driver.
 */
static void joystick_win32_exit(void)
{
   win_remove_all_joysticks();
   win32_joy_num = 0;
}
