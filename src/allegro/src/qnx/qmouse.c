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
 *      QNX mouse driver.
 *
 *      By Angelo Mottola.
 *
 *      Based on Unix/X11 version by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif


static int qnx_mouse_init(void);
static void qnx_mouse_exit(void);
static void qnx_mouse_position(int, int);
static void qnx_mouse_set_range(int, int, int, int);
static void qnx_mouse_get_mickeys(int *, int *);


MOUSE_DRIVER mouse_qnx =
{
   MOUSE_QNX,
   empty_string,
   empty_string,
   "Photon mouse",
   qnx_mouse_init,
   qnx_mouse_exit,
   NULL,       // AL_METHOD(void, poll, (void));
   NULL,       // AL_METHOD(void, timer_poll, (void));
   qnx_mouse_position,
   qnx_mouse_set_range,
   NULL,       // AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   qnx_mouse_get_mickeys,
   NULL,       // AL_METHOD(int,  analyse_data, (AL_CONST char *buffer, int size));
   NULL,       // AL_METHOD(void,  enable_hardware_cursor, (AL_CONST int mode));
   NULL        // AL_METHOD(int,  select_system_cursor, (AL_CONST int cursor));
};


/* global variable */
int qnx_mouse_warped = FALSE;

static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;
static int mymickey_y = 0;



/* qnx_mouse_handler:
 *  Mouse "interrupt" handler for mickey-mode driver.
 */
void qnx_mouse_handler(int x, int y, int z, int buttons)
{
   _mouse_b = buttons;
   
   mymickey_x += x;
   mymickey_y += y;

   _mouse_x += x;
   _mouse_y += y;
   _mouse_z += z;
   
   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx)
       || (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {
      _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);
   }

   _handle_mouse_input();
}



/* qnx_mouse_init:
 *  Initializes the mickey-mode driver.
 */
static int qnx_mouse_init(void)
{
   return 3;
}



/* qnx_mouse_exit:
 *  Shuts down the mickey-mode driver.
 */
static void qnx_mouse_exit(void)
{
}



/* qnx_mouse_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void qnx_mouse_position(int x, int y)
{
   short mx = 0, my = 0;

   pthread_mutex_lock(&qnx_event_mutex);
   
   _mouse_x = x;
   _mouse_y = y;
   
   if (ph_gfx_mode == PH_GFX_WINDOW)
      PtGetAbsPosition(ph_window, &mx, &my);
   
   PhMoveCursorAbs(PhInputGroup(NULL), x + mx, y + my);
   
   mymickey_x = mymickey_y = 0;

   qnx_mouse_warped = TRUE;
   
   pthread_mutex_unlock(&qnx_event_mutex);
}



/* qnx_mouse_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void qnx_mouse_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   pthread_mutex_lock(&qnx_event_mutex);

   _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

   pthread_mutex_unlock(&qnx_event_mutex);
}



/* qnx_mouse_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void qnx_mouse_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}
