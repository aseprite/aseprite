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
 *      Timer routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_TIMER_H
#define ALLEGRO_TIMER_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define TIMERS_PER_SECOND     1193181L
#define SECS_TO_TIMER(x)      ((long)(x) * TIMERS_PER_SECOND)
#define MSEC_TO_TIMER(x)      ((long)(x) * (TIMERS_PER_SECOND / 1000))
#define BPS_TO_TIMER(x)       (TIMERS_PER_SECOND / (long)(x))
#define BPM_TO_TIMER(x)       ((60 * TIMERS_PER_SECOND) / (long)(x))

typedef struct TIMER_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(int,  install_int, (AL_METHOD(void, proc, (void)), long speed));
   AL_METHOD(void, remove_int, (AL_METHOD(void, proc, (void))));
   AL_METHOD(int,  install_param_int, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
   AL_METHOD(void, remove_param_int, (AL_METHOD(void, proc, (void *param)), void *param));
   AL_METHOD(int,  can_simulate_retrace, (void));
   AL_METHOD(void, simulate_retrace, (int enable));
   AL_METHOD(void, rest, (unsigned int tyme, AL_METHOD(void, callback, (void))));
} TIMER_DRIVER;


AL_VAR(TIMER_DRIVER *, timer_driver);
AL_ARRAY(_DRIVER_INFO, _timer_driver_list);

AL_FUNC(int, install_timer, (void));
AL_FUNC(void, remove_timer, (void));

AL_FUNC(int, install_int_ex, (AL_METHOD(void, proc, (void)), long speed));
AL_FUNC(int, install_int, (AL_METHOD(void, proc, (void)), long speed));
AL_FUNC(void, remove_int, (AL_METHOD(void, proc, (void))));

AL_FUNC(int, install_param_int_ex, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
AL_FUNC(int, install_param_int, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
AL_FUNC(void, remove_param_int, (AL_METHOD(void, proc, (void *param)), void *param));

AL_VAR(volatile int, retrace_count);

AL_FUNC(void, rest, (unsigned int tyme));
AL_FUNC(void, rest_callback, (unsigned int tyme, AL_METHOD(void, callback, (void))));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_TIMER_H */

