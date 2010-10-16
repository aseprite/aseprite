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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif                



MOUSE_DRIVER mouse_beos = {
   MOUSE_BEOS,		// int id;
   empty_string,	// char *name;
   empty_string,	// char *desc;
   "Mouse",		// char *ascii_name;
   be_mouse_init,	// AL_METHOD(int, init, (void));
   be_mouse_exit,	// AL_METHOD(void, exit, (void));
   NULL,		// AL_METHOD(void, poll, (void));
   NULL,		// AL_METHOD(void, timer_poll, (void));
   be_mouse_position,	// AL_METHOD(void, position, (int x, int y));
   be_mouse_set_range,	// AL_METHOD(void, set_range, (int x1, int y1, int x2, int y2));
   be_mouse_set_speed,	// AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   be_mouse_get_mickeys,// AL_METHOD(void, get_mickeys, (int *mickeyx, int *mickeyy));
   NULL,                // AL_METHOD(int,  analyse_data, (const char *buffer, int size));
   NULL,                // AL_METHOD(void,  enable_hardware_cursor, (AL_CONST int mode));
   NULL                 // AL_METHOD(int,  select_system_cursor, (AL_CONST int cursor));
};
