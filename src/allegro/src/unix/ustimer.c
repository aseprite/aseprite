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
 *      Timer module for Unix, using SIGALRM.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"



#ifndef ALLEGRO_HAVE_LIBPTHREAD


static int sigalrm_timer_init(void);
static void sigalrm_timer_exit(void);



TIMER_DRIVER timerdrv_unix_sigalrm =
{
   TIMERDRV_UNIX_SIGALRM,
   empty_string,
   empty_string,
   "Unix SIGALRM timer",
   sigalrm_timer_init,
   sigalrm_timer_exit,
   NULL, NULL,		/* install_int, remove_int */
   NULL, NULL,		/* install_param_int, remove_param_int */
   NULL, NULL,		/* can_simulate_retrace, simulate_retrace */
   _unix_rest		/* rest */
};



/* sigalrm_timerint:
 *  Timer "interrupt" handler.
 */
static void sigalrm_timerint(unsigned long interval)
{
   _handle_timer_tick(interval);
}



/* sigalrm_timer_init:
 *  Installs the timer driver.
 */
static int sigalrm_timer_init(void)
{
   if (_unix_bg_man != &_bg_man_sigalrm)
      return -1;

   DISABLE();

   _sigalrm_timer_interrupt_handler = sigalrm_timerint;

   ENABLE();

   return 0;
}



/* sigalrm_timer_exit:
 *  Shuts down the timer driver.
 */
static void sigalrm_timer_exit(void)
{
   DISABLE();

   _sigalrm_timer_interrupt_handler = 0;

   ENABLE();
}


#endif

