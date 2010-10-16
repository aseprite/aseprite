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

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif

#define TIMER_THREAD_PRIORITY 120



thread_id     timer_thread_id      = -1;
volatile bool timer_thread_running = false;



/* timer_thread:
 */
static int32 timer_thread(void *timer_started)
{
   unsigned long delay;

   delay = 0x8000;

   release_sem(*(sem_id *)timer_started);
   
   while(timer_thread_running) {
      snooze((bigtime_t)(delay / 1.193181));
      acquire_sem(_be_sound_stream_lock);
      delay = _handle_timer_tick(delay);
      release_sem(_be_sound_stream_lock);
   }

   return 0;
}



/* be_time_init:
 */
extern "C" int be_time_init(void)
{
   sem_id timer_started;

   timer_started = create_sem(0, "starting timer driver...");

   if(timer_started < 0) {
      goto cleanup;
   }

   timer_thread_id = spawn_thread(timer_thread, "timer driver",
                        TIMER_THREAD_PRIORITY, &timer_started);

   if(timer_thread_id < 0) {
      goto cleanup;
   }

   timer_thread_running = true;
   resume_thread(timer_thread_id);

   return 0;

   cleanup: {
      if (timer_started > 0) {
         delete_sem(timer_started);
      }

      be_time_exit();
      return 1;
   }
}



/* be_time_exit:
 */
extern "C" void be_time_exit(void)
{
   if (timer_thread_id > 0) {
      timer_thread_running = false;
      wait_for_thread(timer_thread_id, &ignore_result);
      timer_thread_id = -1;
   }
}



/* be_time_can_simulate_retrace:
 */
//extern "C" int be_time_can_simulate_retrace(void)
//{
//   return FALSE;
//}



/* be_time_simulate_retrace:
 */
// TODO: See if it will be possible to implement this.
//extern "C" void be_time_simulate_retrace(int enable)
//{
//}



/* be_time_rest:
 */
extern "C" void be_time_rest(unsigned int time, AL_METHOD(void, callback, (void)))
{
   time *= 1000;

   if(callback != NULL) {
      bigtime_t start;
      bigtime_t end;

      start = system_time();
      end   = start + time;

      while(system_time() < end) {
         callback();
      }
   }
   else {
      snooze(time);
   }
}



/* be_time_suspend:
 */
extern "C" void be_time_suspend(void)
{
   if (timer_thread_id > 0) {
      suspend_thread(timer_thread_id);
   }
}



extern "C" void be_time_resume(void)
{
   if (timer_thread_id > 0) {
      resume_thread(timer_thread_id);
   }
}
