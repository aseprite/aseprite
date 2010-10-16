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
 *      X-Windows mouse module.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"
#include <X11/cursorfont.h>


/* TRUE if the requested mouse range extends beyond the regular
 * (0, 0, SCREEN_W-1, SCREEN_H-1) range. This is aimed at detecting
 * whether the user mouse coordinates are relative to the 'screen'
 * bitmap (semantics associated with scrolling) or to the video page
 * currently being displayed (semantics associated with page flipping).
 * We cannot differentiate them properly because both use the same
 * scroll_screen() method.
 */
int _xwin_mouse_extended_range = FALSE;

static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;
static int mymickey_y = 0;

static int mouse_mult = -1;       /* mouse acceleration multiplier */
static int mouse_div = -1;        /* mouse acceleration divisor */
static int mouse_threshold = -1;  /* mouse acceleration threshold */

static int last_xspeed = -1;      /* latest set_mouse_speed() settings */
static int last_yspeed = -1;



static int _xwin_mousedrv_init(void);
static void _xwin_mousedrv_exit(void);
static void _xwin_mousedrv_position(int x, int y);
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2);
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed);
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy);
static int _xwin_select_system_cursor(AL_CONST int cursor);

static void _xwin_set_mouse_speed(int xspeed, int yspeed);

static MOUSE_DRIVER mouse_xwin =
{
   MOUSE_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows mouse",
   _xwin_mousedrv_init,
   _xwin_mousedrv_exit,
   NULL,
   NULL,
   _xwin_mousedrv_position,
   _xwin_mousedrv_set_range,
   _xwin_mousedrv_set_speed,
   _xwin_mousedrv_get_mickeys,
   NULL,
   _xwin_enable_hardware_cursor,
   _xwin_select_system_cursor
};



/* list the available drivers */
_DRIVER_INFO _xwin_mouse_driver_list[] =
{
   {  MOUSE_XWINDOWS, &mouse_xwin, TRUE  },
   {  0,              NULL,        0     }
};



/* _xwin_mousedrv_handler:
 *  Mouse "interrupt" handler for mickey-mode driver.
 */
static void _xwin_mousedrv_handler(int x, int y, int z, int w, int buttons)
{
   _mouse_b = buttons;

   mymickey_x += x;
   mymickey_y += y;

   _mouse_x += x;
   _mouse_y += y;
   _mouse_z += z;
   _mouse_w += w;

   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx)
       || (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {
      _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);
   }

   _handle_mouse_input();
}



/* _xwin_mousedrv_init:
 *  Initializes the mickey-mode driver.
 */
static int _xwin_mousedrv_init(void)
{
   int num_buttons;
   unsigned char map[8];

   num_buttons = _xwin_get_pointer_mapping(map, sizeof(map));
   num_buttons = CLAMP(2, num_buttons, 3);

   last_xspeed = -1;
   last_yspeed = -1;

   XLOCK();

   _xwin_mouse_interrupt = _xwin_mousedrv_handler;

   XUNLOCK();

   return num_buttons;
}



/* _xwin_mousedrv_exit:
 *  Shuts down the mickey-mode driver.
 */
static void _xwin_mousedrv_exit(void)
{
   XLOCK();

   if (mouse_mult >= 0)
      XChangePointerControl(_xwin.display, 1, 1, mouse_mult,
         mouse_div, mouse_threshold);

   _xwin_mouse_interrupt = 0;

   XUNLOCK();
}



/* _xwin_mousedrv_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void _xwin_mousedrv_position(int x, int y)
{
   XLOCK();

   _mouse_x = x;
   _mouse_y = y;

   mymickey_x = mymickey_y = 0;

   if (_xwin.hw_cursor_ok)
      XWarpPointer(_xwin.display, _xwin.window, _xwin.window, 0, 0, 
                   _xwin.window_width, _xwin.window_height, x, y);
   XUNLOCK();

   _xwin_set_warped_mouse_mode(FALSE);
}



/* _xwin_mousedrv_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   if ((mouse_maxx >= SCREEN_W) || (mouse_maxy >= SCREEN_H))
      _xwin_mouse_extended_range = TRUE;
   else
      _xwin_mouse_extended_range = FALSE;

   XLOCK();

   _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

   XUNLOCK();
}



/* _xwin_mousedrv_set_speed:
 *  Sets the speed of the mouse cursor.  We don't set the speed if the cursor
 *  isn't in the window, but we remember the setting so it will be set the
 *  next time the cursor enters the window.
 */
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed)
{
   if (_mouse_on) {
      _xwin_set_mouse_speed(xspeed, yspeed);
   }

   last_xspeed = xspeed;
   last_yspeed = yspeed;
}



/* _xwin_mousedrv_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;

   _xwin_set_warped_mouse_mode(TRUE);
}



/* _xwin_select_system_cursor:
 *  Select an OS native cursor 
 */
static int _xwin_select_system_cursor(AL_CONST int cursor)
{
   switch(cursor) {
      case MOUSE_CURSOR_ARROW:
         _xwin.cursor_shape = XC_left_ptr;
         break;
      case MOUSE_CURSOR_BUSY:
         _xwin.cursor_shape = XC_watch;
         break;
      case MOUSE_CURSOR_QUESTION:
         _xwin.cursor_shape = XC_question_arrow;
         break;
      case MOUSE_CURSOR_EDIT:
         _xwin.cursor_shape = XC_xterm;
         break;
      default:
         return 0;
   }

   XLOCK();

   if (_xwin.cursor != None) {
      XUndefineCursor(_xwin.display, _xwin.window);
      XFreeCursor(_xwin.display, _xwin.cursor);
   }

   _xwin.cursor = XCreateFontCursor(_xwin.display, _xwin.cursor_shape);
   XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);

   XUNLOCK();

   return cursor;
}



/* _xwin_set_mouse_speed:
 *  The actual function that sets the speed of the mouse cursor.
 *  Each step slows down or speeds the mouse up by 0.5x.
 */
static void _xwin_set_mouse_speed(int xspeed, int yspeed)
{
   int speed;
   int hundredths;

   XLOCK();

   if (mouse_mult < 0)
      XGetPointerControl(_xwin.display, &mouse_mult, &mouse_div,
         &mouse_threshold);

   speed = MAX(1, (xspeed + yspeed) / 2);

   if (mouse_div == 0)
      hundredths = mouse_mult * 100;
   else
      hundredths = (mouse_mult * 100 / mouse_div);
   hundredths -= (speed - 2) * 50;
   if (hundredths < 0)
      hundredths = 0;

   XChangePointerControl(_xwin.display, 1, 1, hundredths,
      100, mouse_threshold);

   XUNLOCK();
}



/* _xwin_mouse_leave_notify:
 *  Reset the mouse speed to its original value when the cursor leave the
 *  Allegro window.
 */
void _xwin_mouse_leave_notify(void)
{
   if (mouse_mult >= 0) {
      XLOCK();
      XChangePointerControl(_xwin.display, 1, 1, mouse_mult,
         mouse_div, mouse_threshold);
      XUNLOCK();
   }
}



/* _xwin_mouse_enter_notify:
 *  Restore the mouse speed setting when the mouse cursor re-enters the
 *  Allegro window.
 */
void _xwin_mouse_enter_notify(void)
{
   if (last_xspeed >= 0) {
      _xwin_set_mouse_speed(last_xspeed, last_yspeed);
   }
}
