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
 *      Timer interrupt routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Synchronization added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include <time.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

TIMER_DRIVER *timer_driver = NULL;        /* the active driver */

int _timer_installed = FALSE;

TIMER_QUEUE _timer_queue[MAX_TIMERS];     /* list of active callbacks */

volatile int retrace_count = 0;           /* used for retrace syncing */
void (*retrace_proc)(void) = NULL;

long _vsync_speed = BPS_TO_TIMER(70);     /* retrace speed */

static long vsync_counter;                /* retrace position counter */

int _timer_use_retrace = FALSE;           /* are we synced to the retrace? */

volatile int _retrace_hpp_value = -1;     /* to set during next retrace */

#ifdef ALLEGRO_MULTITHREADED
static void *timer_mutex = NULL;          /* global timer mutex */
   /* Why we need timer_mutex:
    *
    * 1. So _timer_queue[] is not changed in the middle of
    * _handle_timer_tick's loop, possibly causing a null pointer
    * dereference or corruption;
    *
    * 2. So that after "remove_int(my_callback);" returns, my_callback
    * is guaranteed not to be in the _timer_queue and will not be
    * called from the timer thread.  Eric explains it more clearly:
    *
    *   my_callback can have been fired but has not completed yet; context
    *   switch; the main thread returns from remove_int and set my_pointer
    *   to 0; context switch; the callback dereferences my_pointer.  Ergo
    *   remove_int must wait for all callbacks to have completed before
    *   returning.
    */
#else
static int timer_semaphore = FALSE;       /* reentrant interrupt? */
#endif

static volatile long timer_delay = 0;     /* lost interrupt rollover */



/* _handle_timer_tick:
 *  Called by the driver to handle a timer tick.
 */
long _handle_timer_tick(int interval)
{
   long new_delay = 0x8000;
   long d;
   int i;

   timer_delay += interval;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->lock_mutex(timer_mutex);
#else
   /* reentrant interrupt? */
   if (timer_semaphore)
      return 0x2000;

   timer_semaphore = TRUE;
#endif

   d = timer_delay;

   /* deal with retrace synchronisation */
   vsync_counter -= d; 

   while (vsync_counter <= 0) {
      vsync_counter += _vsync_speed;
      retrace_count++;
      if (retrace_proc)
	 retrace_proc();
   }

   /* process the user callbacks */
   for (i=0; i<MAX_TIMERS; i++) { 
      if (((_timer_queue[i].proc) || (_timer_queue[i].param_proc)) &&
	  (_timer_queue[i].speed > 0)) {

	 _timer_queue[i].counter -= d;

	 while ((_timer_queue[i].counter <= 0) && 
		((_timer_queue[i].proc) || (_timer_queue[i].param_proc)) && 
		(_timer_queue[i].speed > 0)) {
	    _timer_queue[i].counter += _timer_queue[i].speed;
	    if (_timer_queue[i].param_proc)
	       _timer_queue[i].param_proc(_timer_queue[i].param);
	    else
	       _timer_queue[i].proc();
	 }

	 if ((_timer_queue[i].counter > 0) && 
	     ((_timer_queue[i].proc) || (_timer_queue[i].param_proc)) && 
	     (_timer_queue[i].counter < new_delay)) {
	    new_delay = _timer_queue[i].counter;
	 }
      }
   }

   timer_delay -= d;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->unlock_mutex(timer_mutex);
#else
   timer_semaphore = FALSE;
#endif

#ifdef ALLEGRO_WINDOWS
   /* fudge factor to prevent interrupts from coming too close to each other */
   if (new_delay < MSEC_TO_TIMER(1))
      new_delay = MSEC_TO_TIMER(1);
#endif

   return new_delay;
}

END_OF_FUNCTION(_handle_timer_tick);



/* simple interrupt handler for the rest() function */
static volatile long rest_count;

static void rest_int(void)
{
   rest_count--;
}

END_OF_STATIC_FUNCTION(rest_int);



/* rest_callback:
 *  Waits for time milliseconds.
 */
void rest_callback(unsigned int time, void (*callback)(void))
{
   if (!time) {
      ASSERT(system_driver);
      if (system_driver->yield_timeslice)
         system_driver->yield_timeslice();
      return;
   }
   if (timer_driver) {
      if (timer_driver->rest) {
	 timer_driver->rest(time, callback);
      }
      else {
	 rest_count = time;

	 if (install_int(rest_int, 1) < 0)
	    return;

	 do {
	    if (callback)
	       callback();
	    else
	       rest(0);

	 } while (rest_count > 0);

	 remove_int(rest_int);
      }
   }
   else {
      time = clock() + MIN(time * CLOCKS_PER_SEC / 1000, 2);
      do {
         rest(0);
      } while (clock() < (clock_t)time);
   }
}



/* rest:
 *  Waits for time milliseconds.
 */
void rest(unsigned int time)
{
   rest_callback(time, NULL);
}



/* timer_can_simulate_retrace: [deprecated but used internally]
 *  Checks whether the current driver is capable of a video retrace
 *  syncing mode.
 */
int timer_can_simulate_retrace(void)
{
   if ((timer_driver) && (timer_driver->can_simulate_retrace))
      return timer_driver->can_simulate_retrace();
   else
      return FALSE;
}



/* timer_simulate_retrace: [deprecated but used internally]
 *  Turns retrace simulation on or off, and if turning it on, calibrates
 *  the retrace timer.
 */
void timer_simulate_retrace(int enable)
{
   if (!timer_can_simulate_retrace())
      return;

   timer_driver->simulate_retrace(enable);
}



/* timer_is_using_retrace: [deprecated]
 *  Tells the user whether the current driver is providing a retrace
 *  sync.
 */
int timer_is_using_retrace(void)
{
   return _timer_use_retrace;
}



/* find_timer_slot:
 *  Searches the list of user timer callbacks for a specified function, 
 *  returning the position at which it was found, or -1 if it isn't there.
 */
static int find_timer_slot(void (*proc)(void))
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if (_timer_queue[x].proc == proc)
	 return x;

   return -1;
}

END_OF_STATIC_FUNCTION(find_timer_slot);



/* find_param_timer_slot:
 *  Searches the list of user timer callbacks for a specified paramater 
 *  function, returning the position at which it was found, or -1 if it 
 *  isn't there.
 */
static int find_param_timer_slot(void (*proc)(void *param), void *param)
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if ((_timer_queue[x].param_proc == proc) && (_timer_queue[x].param == param))
	 return x;

   return -1;
}

END_OF_STATIC_FUNCTION(find_param_timer_slot);



/* find_empty_timer_slot:
 *  Searches the list of user timer callbacks for an empty slot.
 */
static int find_empty_timer_slot(void)
{
   int x;

   for (x=0; x<MAX_TIMERS; x++)
      if ((!_timer_queue[x].proc) && (!_timer_queue[x].param_proc))
	 return x;

   return -1;
}

END_OF_STATIC_FUNCTION(find_empty_timer_slot);



/* install_timer_int:
 *  Installs a function into the list of user timers, or if it is already 
 *  installed, adjusts its speed. This function will be called once every 
 *  speed timer ticks. Returns a negative number if there was no room to 
 *  add a new routine.
 */
static int install_timer_int(void *proc, void *param, long speed, int param_used)
{
   int x;

   if (!timer_driver) {                   /* make sure we are installed */
      if (install_timer() != 0)
	 return -1;
   }

   if (param_used) {
      if (timer_driver->install_param_int) 
	 return timer_driver->install_param_int((void (*)(void *))proc, param, speed);

      x = find_param_timer_slot((void (*)(void *))proc, param);
   }
   else {
      if (timer_driver->install_int) 
	 return timer_driver->install_int((void (*)(void))proc, speed);

      x = find_timer_slot((void (*)(void))proc); 
   }

   if (x < 0)
      x = find_empty_timer_slot();

   if (x < 0)
      return -1;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->lock_mutex(timer_mutex);
#endif

   if ((proc == _timer_queue[x].proc) || (proc == _timer_queue[x].param_proc)) { 
      _timer_queue[x].counter -= _timer_queue[x].speed;
      _timer_queue[x].counter += speed;
   }
   else {
      _timer_queue[x].counter = speed;
      if (param_used) {
	 _timer_queue[x].param = param;
	 _timer_queue[x].param_proc = proc;
      }
      else
	 _timer_queue[x].proc = proc;
   }

   _timer_queue[x].speed = speed;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->unlock_mutex(timer_mutex);
#endif

   return 0;
}

END_OF_STATIC_FUNCTION(install_timer_int);



/* install_int:
 *  Wrapper for install_timer_int, which takes the speed in milliseconds,
 *  no paramater.
 */
int install_int(void (*proc)(void), long speed)
{
   return install_timer_int((void *)proc, NULL, MSEC_TO_TIMER(speed), FALSE);
}

END_OF_FUNCTION(install_int);



/* install_int_ex:
 *  Wrapper for install_timer_int, which takes the speed in timer ticks,
 *  no paramater.
 */
int install_int_ex(void (*proc)(void), long speed)
{
   return install_timer_int((void *)proc, NULL, speed, FALSE);
}

END_OF_FUNCTION(install_int_ex);



/* install_param_int:
 *  Wrapper for install_timer_int, which takes the speed in milliseconds,
 *  with paramater.
 */
int install_param_int(void (*proc)(void *param), void *param, long speed)
{
   return install_timer_int((void *)proc, param, MSEC_TO_TIMER(speed), TRUE);
}

END_OF_FUNCTION(install_param_int);



/* install_param_int_ex:
 *  Wrapper for install_timer_int, which takes the speed in timer ticks,
 *  no paramater.
 */
int install_param_int_ex(void (*proc)(void *param), void *param, long speed)
{
   return install_timer_int((void *)proc, param, speed, TRUE);
}

END_OF_FUNCTION(install_param_int_ex);



/* remove_timer_int:
 *  Removes a function from the list of user timers.
 */
static void remove_timer_int(void *proc, void *param, int param_used)
{
   int x;

   if (param_used) {
      if ((timer_driver) && (timer_driver->remove_param_int)) {
	 timer_driver->remove_param_int((void (*)(void *))proc, param);
	 return;
      }

      x = find_param_timer_slot((void (*)(void *))proc, param);
   }
   else {
      if ((timer_driver) && (timer_driver->remove_int)) {
	 timer_driver->remove_int((void (*)(void))proc);
	 return;
      }

      x = find_timer_slot((void (*)(void))proc); 
   }

   if (x < 0)
      return;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->lock_mutex(timer_mutex);
#endif

   _timer_queue[x].proc = NULL;
   _timer_queue[x].param_proc = NULL;
   _timer_queue[x].param = NULL;
   _timer_queue[x].speed = 0;
   _timer_queue[x].counter = 0;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->unlock_mutex(timer_mutex);
#endif
}

END_OF_FUNCTION(remove_timer_int);



/* remove_int:
 *  Wrapper for remove_timer_int, without parameters.
 */
void remove_int(void (*proc)(void))
{
   remove_timer_int((void *)proc, NULL, FALSE);
}

END_OF_FUNCTION(remove_int);



/* remove_param_int:
 *  Wrapper for remove_timer_int, with parameters.
 */
void remove_param_int(void (*proc)(void *param), void *param)
{
   remove_timer_int((void *)proc, param, TRUE);
}

END_OF_FUNCTION(remove_param_int);



/* clear_timer_queue:
 *  Clears the timer queue.
 */
static void clear_timer_queue(void)
{
   int i;

   for (i=0; i<MAX_TIMERS; i++) {
      _timer_queue[i].proc = NULL;
      _timer_queue[i].param_proc = NULL;
      _timer_queue[i].param = NULL;
      _timer_queue[i].speed = 0;
      _timer_queue[i].counter = 0;
   }
}



/* install_timer:
 *  Installs the timer interrupt handler. You must do this before installing
 *  any user timer routines. You must set up the timer before trying to 
 *  display a mouse pointer or using any of the GUI routines.
 */
int install_timer(void)
{
   _DRIVER_INFO *driver_list;
   int i;

   if (timer_driver)
      return 0;

   clear_timer_queue();

   retrace_proc = NULL;
   vsync_counter = BPS_TO_TIMER(70);
   _timer_use_retrace = FALSE;
   _retrace_hpp_value = -1;
   timer_delay = 0;

   LOCK_VARIABLE(timer_driver);
   LOCK_VARIABLE(timer_delay);
   LOCK_VARIABLE(_timer_queue);
   LOCK_VARIABLE(timer_semaphore);
   LOCK_VARIABLE(vsync_counter);
   LOCK_VARIABLE(_timer_use_retrace);
   LOCK_VARIABLE(_retrace_hpp_value);
   LOCK_VARIABLE(retrace_count);
   LOCK_VARIABLE(retrace_proc);
   LOCK_VARIABLE(rest_count);
   LOCK_FUNCTION(rest_int);
   LOCK_FUNCTION(_handle_timer_tick);
   LOCK_FUNCTION(find_timer_slot);
   LOCK_FUNCTION(find_param_timer_slot);
   LOCK_FUNCTION(find_empty_timer_slot);
   LOCK_FUNCTION(install_timer_int);
   LOCK_FUNCTION(install_int);
   LOCK_FUNCTION(install_int_ex);
   LOCK_FUNCTION(install_param_int);
   LOCK_FUNCTION(install_param_int_ex);
   LOCK_FUNCTION(remove_int);
   LOCK_FUNCTION(remove_param_int);

   /* autodetect a driver */
   if (system_driver->timer_drivers)
      driver_list = system_driver->timer_drivers();
   else
      driver_list = _timer_driver_list;

#ifdef ALLEGRO_MULTITHREADED
   timer_mutex = system_driver->create_mutex();
   if (!timer_mutex)
      return -1;
#endif

   for (i=0; driver_list[i].driver; i++) {
      timer_driver = driver_list[i].driver;
      timer_driver->name = timer_driver->desc = get_config_text(timer_driver->ascii_name);
      if (timer_driver->init() == 0)
	 break;
   }

   if (!driver_list[i].driver) {
#ifdef ALLEGRO_MULTITHREADED
      system_driver->destroy_mutex(timer_mutex);
      timer_mutex = NULL;
#endif
      timer_driver = NULL;
      return -1;
   }

   _add_exit_func(remove_timer, "remove_timer");
   _timer_installed = TRUE;

   return 0;
}



/* remove_timer:
 *  Removes our timer handler and resets the BIOS clock. You don't normally
 *  need to call this, because allegro_exit() will do it for you.
 */
void remove_timer(void)
{
   if (!timer_driver)
      return;

   _timer_use_retrace = FALSE;

   timer_driver->exit();
   timer_driver = NULL;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->destroy_mutex(timer_mutex);
   timer_mutex = NULL;
#endif

   /* make sure subsequent remove_int() calls don't crash */
   clear_timer_queue();

   _remove_exit_func(remove_timer);
   _timer_installed = FALSE;
}

