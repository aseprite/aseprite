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
 *      Windows timer driver. 
 *
 *      By Stefan Schimanski.
 *
 *      Enhanced low performance driver by Eric Botcazou.
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
   #include <process.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wtimer INFO: "
#define PREFIX_W                "al-wtimer WARNING: "
#define PREFIX_E                "al-wtimer ERROR: "


static TIMER_DRIVER timer_win32_high_perf;
static TIMER_DRIVER timer_win32_low_perf;


_DRIVER_INFO _timer_driver_list[] =
{
   {TIMER_WIN32_HIGH_PERF, &timer_win32_high_perf, TRUE},
   {TIMER_WIN32_LOW_PERF,  &timer_win32_low_perf,  TRUE},
   {0, NULL, 0}
};


static int tim_win32_high_perf_init(void);
static int tim_win32_low_perf_init(void);
static void tim_win32_exit(void);
static void tim_win32_rest(unsigned int time, AL_METHOD(void, callback, (void)));


static TIMER_DRIVER timer_win32_high_perf =
{
   TIMER_WIN32_HIGH_PERF,
   empty_string,
   empty_string,
   "Win32 high performance timer",
   tim_win32_high_perf_init,
   tim_win32_exit,
   NULL, NULL, NULL, NULL, NULL, NULL,
   tim_win32_rest
};


static TIMER_DRIVER timer_win32_low_perf =
{
   TIMER_WIN32_LOW_PERF,
   empty_string,
   empty_string,
   "Win32 low performance timer",
   tim_win32_low_perf_init,
   tim_win32_exit,
   NULL, NULL, NULL, NULL, NULL, NULL,
   tim_win32_rest
};


static HANDLE timer_thread = NULL;       /* dedicated timer thread      */
static HANDLE timer_stop_event = NULL;   /* thread termination event    */

/* high performance driver */
static LARGE_INTEGER counter_freq;
static LARGE_INTEGER counter_per_msec;

/* unit conversion */
#define COUNTER_TO_MSEC(x) ((unsigned long)(x / counter_per_msec.QuadPart))
#define COUNTER_TO_TIMER(x) ((unsigned long)(x * TIMERS_PER_SECOND / counter_freq.QuadPart))
#define MSEC_TO_COUNTER(x) (x * counter_per_msec.QuadPart)
#define TIMER_TO_COUNTER(x) (x * counter_freq.QuadPart / TIMERS_PER_SECOND)
#define TIMER_TO_MSEC(x) ((unsigned long)(x) / (TIMERS_PER_SECOND / 1000))



/* tim_win32_high_perf_thread:
 *  Thread loop function for the high performance driver.
 */
static void tim_win32_high_perf_thread(void *unused)
{
   DWORD result;
   unsigned long delay = 0x8000;
   LARGE_INTEGER curr_counter;
   LARGE_INTEGER prev_tick;
   LARGE_INTEGER diff_counter;

   /* init thread */
   _win_thread_init();

   /* get initial counter */
   QueryPerformanceCounter(&prev_tick);

   while (TRUE) {
      if (!_win_app_foreground) {
	 /* restart counter if the thread was blocked */
	 if (_win_thread_switch_out())
	    QueryPerformanceCounter(&prev_tick);
      }

      /* get current counter */
      QueryPerformanceCounter(&curr_counter);

      /* get counter units till next tick */
      diff_counter.QuadPart = curr_counter.QuadPart - prev_tick.QuadPart;

      /* save current counter for next tick */
      prev_tick.QuadPart = curr_counter.QuadPart;

      /* call timer proc */
      delay = _handle_timer_tick(COUNTER_TO_TIMER(diff_counter.QuadPart));

      /* wait calculated time */
      result = WaitForSingleObject(timer_stop_event, TIMER_TO_MSEC(delay));
      if (result != WAIT_TIMEOUT) {
	 _win_thread_exit();
	 return;
      }
   }
}



/* tim_win32_low_perf_thread:
 *  Thread loop function for the low performance driver.
 */
static void tim_win32_low_perf_thread(void *unused)
{
   DWORD result;
   unsigned long delay = 0x8000;
   DWORD prev_time;  
   DWORD curr_time;  /* in milliseconds */
   DWORD diff_time;

   /* init thread */
   _win_thread_init();

   /* get initial time */
   prev_time = timeGetTime();

   while (TRUE) {
      if (!_win_app_foreground) {
	 /* restart time if the thread was blocked */
	 if (_win_thread_switch_out())
	    prev_time = timeGetTime();
      }

      /* get current time */
      curr_time = timeGetTime();

      /* get time till next tick */
      diff_time = curr_time - prev_time;

      /* save current time for next tick */
      prev_time = curr_time;

      /* call timer proc */
      delay = _handle_timer_tick(MSEC_TO_TIMER(diff_time));

      /* wait calculated time */
      result = WaitForSingleObject(timer_stop_event, TIMER_TO_MSEC(delay));
      if (result != WAIT_TIMEOUT) {
	 _win_thread_exit();
	 return;
      }
   }
}



/* tim_win32_high_perf_init:
 *  Initializes the high performance driver.
 */
static int tim_win32_high_perf_init(void)
{
   if (!QueryPerformanceFrequency(&counter_freq)) {
      _TRACE(PREFIX_W "High performance timer not supported\n");
      return -1;
   }

   /* setup high performance counter */
   counter_per_msec.QuadPart = counter_freq.QuadPart / 1000;

   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   /* start timer thread */
   timer_thread = (void *)_beginthread(tim_win32_high_perf_thread, 0, 0);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread, THREAD_PRIORITY_TIME_CRITICAL);

   return 0;
}



/* tim_win32_low_perf_init:
 *  Initializes the low performance driver.
 */
static int tim_win32_low_perf_init(void)
{
   /* create thread termination event */
   timer_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   /* start timer thread */
   timer_thread = (void *)_beginthread(tim_win32_low_perf_thread, 0, 0);

   /* increase priority of timer thread */
   SetThreadPriority(timer_thread, THREAD_PRIORITY_TIME_CRITICAL);

   return 0;
}



/* tim_win32_exit:
 *  Shuts down either timer driver.
 */
static void tim_win32_exit(void)
{
   /* acknowledge that the thread should stop */
   SetEvent(timer_stop_event);

   /* wait until thread has ended */
   while (WaitForSingleObject(timer_thread, 100) == WAIT_TIMEOUT);

   /* thread has ended, now we can release all resources */
   CloseHandle(timer_stop_event);
}



/* tim_win32_rest:
 *  Rests the specified amount of milliseconds.
 */
static void tim_win32_rest(unsigned int time, AL_METHOD(void, callback, (void)))
{
   unsigned int start;
   unsigned int ms = time;

   if (callback) {
      start = timeGetTime();
      while (timeGetTime() - start < ms)
         (*callback)();
   }
   else {
      Sleep(ms);
   }
}
