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



TIMER_DRIVER timer_beos = {
   TIMER_BEOS,		// int id;
   empty_string,	// char *name;
   empty_string,	// char *desc;
   "Timer",		// char *ascii_name;
   be_time_init,	// AL_METHOD(int, init, (void));
   be_time_exit,	// AL_METHOD(void, exit, (void));
   NULL, 		// AL_METHOD(int, install_int, (AL_METHOD(void, proc, (void)), long speed));
   NULL,		// AL_METHOD(void, remove_int, (AL_METHOD(void, proc, (void))));
   NULL,		// AL_METHOD(int, install_param_int, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
   NULL,		// AL_METHOD(void, remove_param_int, (AL_METHOD(void, proc, (void *param)), void *param));
   NULL,		// AL_METHOD(int, can_simulate_retrace, (void));
   NULL,		// AL_METHOD(void, simulate_retrace, (int enable));
   be_time_rest,	// AL_METHOD(void, rest, (long time, AL_METHOD(void, callback, (void))));
};
