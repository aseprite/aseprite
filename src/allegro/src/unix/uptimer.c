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
 *      Timer module for Unix, using pthreads.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"


#ifdef ALLEGRO_HAVE_LIBPTHREAD

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>


/* See hack later.  */
#ifdef ALLEGRO_LINUX_VGA
#ifdef ALLEGRO_HAVE_SYS_IO_H
/* iopl() exists in here instead of unistd.h in glibc */
#include <sys/io.h>
#endif
#include "allegro/platform/aintlnx.h"
#endif


#define TIMER_TO_USEC(x)  ((long)((x) / 1.193181))
#define USEC_TO_TIMER(x)  ((long)((x) * (TIMERS_PER_SECOND / 1000000.)))


static int ptimer_init(void);
static void ptimer_exit(void);



TIMER_DRIVER timerdrv_unix_pthreads =
{
   TIMERDRV_UNIX_PTHREADS,
   empty_string,
   empty_string,
   "Unix pthreads timers",
   ptimer_init,
   ptimer_exit,
   NULL, NULL,		/* install_int, remove_int */
   NULL, NULL,		/* install_param_int, remove_param_int */
   NULL, NULL,		/* can_simulate_retrace, simulate_retrace */
   _unix_rest		/* rest */
};



static pthread_t thread;
static int thread_alive;



static void block_all_signals(void)
{
#ifndef ALLEGRO_MACOSX
   sigset_t mask;
   sigfillset(&mask);
   pthread_sigmask(SIG_BLOCK, &mask, NULL);
#endif
}



/* ptimer_thread_func:
 *  The timer thread.
 */
static void *ptimer_thread_func(void *unused)
{
   struct timeval old_time;
   struct timeval new_time;
   struct timeval delay;
   long interval = 0x8000;
#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
   struct timespec old_time_ns;
   struct timespec new_time_ns;
   int clock_monotonic;
#endif

   block_all_signals();

#ifdef ALLEGRO_LINUX_VGA
   /* privileges hack for Linux:
    *  One of the jobs of the timer thread is to update the mouse pointer
    *  on screen.  When using the Mode-X driver under Linux console, this
    *  involves selecting different planes (in modexgfx.s), which requires
    *  special priviledges.
    */
   if ((system_driver == &system_linux) && (__al_linux_have_ioperms)) {
      seteuid(0);
      iopl(3);
      seteuid(getuid());
   }
#endif

#ifdef ALLEGRO_QNX
   /* thread priority adjustment for QNX:
    *  The timer thread is set to the highest relative priority.
    *  (see the comment in src/qnx/qsystem.c about the scheduling policy)
    */
   {
      struct sched_param sparam;
      int spolicy;
   
      if (pthread_getschedparam(pthread_self(), &spolicy, &sparam) == EOK) {
         sparam.sched_priority += 4;
         pthread_setschedparam(pthread_self(), spolicy, &sparam);
      }
   }
#endif

#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
   /* clock_gettime(CLOCK_MONOTONIC, ...) is preferable to gettimeofday() in
    * case the system time is changed while the program is running.
    * CLOCK_MONOTONIC is not available on all systems.
    */
   clock_monotonic = (clock_gettime(CLOCK_MONOTONIC, &old_time_ns) == 0);
#endif

   gettimeofday(&old_time, 0);

   while (thread_alive) {
      /* `select' is more accurate than `usleep' on my system.  */
      delay.tv_sec = interval / TIMERS_PER_SECOND;
      delay.tv_usec = TIMER_TO_USEC(interval) % 1000000L;
      select(0, NULL, NULL, NULL, &delay);

      /* Calculate actual time elapsed.  */
#ifdef ALLEGRO_HAVE_POSIX_MONOTONIC_CLOCK
      if (clock_monotonic) {
         clock_gettime(CLOCK_MONOTONIC, &new_time_ns);
         interval = USEC_TO_TIMER(
            (new_time_ns.tv_sec - old_time_ns.tv_sec) * 1000000L +
            (new_time_ns.tv_nsec - old_time_ns.tv_nsec) / 1000);
         old_time_ns = new_time_ns;
      }
      else
#endif
      {
         gettimeofday(&new_time, 0);
         interval = USEC_TO_TIMER(
            (new_time.tv_sec - old_time.tv_sec) * 1000000L +
            (new_time.tv_usec - old_time.tv_usec));
         old_time = new_time;
      }

      /* Handle a tick.  */
      interval = _handle_timer_tick(interval);
   }

   return NULL;
}



/* ptimer_init:
 *  Installs the timer thread.
 */
static int ptimer_init(void)
{
   thread_alive = 1;

   if (pthread_create(&thread, NULL, ptimer_thread_func, NULL) != 0) {
      thread_alive = 0;
      return -1;
   }

   return 0;
}



/* ptimer_exit:
 *  Shuts down the timer thread.
 */
static void ptimer_exit(void)
{
   if (thread_alive) {
      thread_alive = 0;
      pthread_join(thread, NULL);
   }
}


#endif
