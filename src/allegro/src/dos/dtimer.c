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
 *      DOS timer module.
 *
 *      By Shawn Hargreaves.
 *
 *      Accuracy improvements by Neil Townsend.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* Error factor for retrace syncing. Reduce this to spend less time waiting
 * for the retrace, increase it to reduce the chance of missing retraces.
 */
#define VSYNC_MARGIN             1024

#define TIMER_INT                8

#define LOVEBILL_TIMER_SPEED     BPS_TO_TIMER(200)

#define REENTRANT_RECALL_GAP     2500

static long bios_counter;                 /* keep BIOS time up to date */

static long timer_delay;                  /* how long between interrupts */

static int timer_semaphore = FALSE;       /* reentrant interrupt? */

static int timer_clocking_loss = 0;       /* unmeasured time that gets lost */

static long vsync_counter;                /* retrace position counter */
static long vsync_speed;                  /* retrace speed */



/* this driver is locked to a fixed rate, which works under win95 etc. */
int fixed_timer_init(void);
void fixed_timer_exit(void);


TIMER_DRIVER timedrv_fixed_rate =
{
   TIMEDRV_FIXED_RATE,
   empty_string,
   empty_string,
   "Fixed-rate timer",
   fixed_timer_init,
   fixed_timer_exit,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL
};



/* this driver varies the speed on the fly, which only works in clean DOS */
int var_timer_init(void);
void var_timer_exit(void);
int var_timer_can_simulate_retrace(void);
void var_timer_simulate_retrace(int enable);


TIMER_DRIVER timedrv_variable_rate =
{
   TIMEDRV_VARIABLE_RATE,
   empty_string,
   empty_string,
   "Variable-rate timer",
   var_timer_init,
   var_timer_exit,
   NULL, NULL, NULL, NULL,
   var_timer_can_simulate_retrace,
   var_timer_simulate_retrace,
   NULL
};



/* list the available drivers */
_DRIVER_INFO _timer_driver_list[] =
{
   { TIMEDRV_VARIABLE_RATE,   &timedrv_variable_rate,    TRUE  },
   { TIMEDRV_FIXED_RATE,      &timedrv_fixed_rate,       TRUE  },
   { 0,                       NULL,                      0     }
};



/* set_timer:
 *  Sets the delay time for PIT channel 1 in one-shot mode.
 */
static INLINE void set_timer(long time)
{
    outportb(0x43, 0x30);
    outportb(0x40, time & 0xff);
    outportb(0x40, time >> 8);
}



/* set_timer_rate:
 *  Sets the delay time for PIT channel 1 in cycle mode.
 */
static INLINE void set_timer_rate(long time)
{
    outportb(0x43, 0x34);
    outportb(0x40, time & 0xff);
    outportb(0x40, time >> 8);
}



/* read_timer:
 *  Reads the elapsed time from PIT channel 1.
 */
static INLINE long read_timer(void)
{
   long x;

   outportb(0x43, 0x00);
   x = inportb(0x40);
   x += inportb(0x40) << 8;

   return (0xFFFF - x + 1) & 0xFFFF;
}



/* fixed_timer_handler:
 *  Interrupt handler for the fixed-rate timer driver.
 */
static int fixed_timer_handler(void)
{
   int bios;

   bios_counter -= LOVEBILL_TIMER_SPEED; 

   if (bios_counter <= 0) {
      bios_counter += 0x10000;
      bios = TRUE;
   }
   else {
      outportb(0x20, 0x20);
      ENABLE();
      bios = FALSE;
   }

   _handle_timer_tick(LOVEBILL_TIMER_SPEED);

   if (!bios)
      DISABLE();

   return bios;
}

END_OF_STATIC_FUNCTION(fixed_timer_handler);



/* fixed_timer_init:
 *  Installs the fixed-rate timer driver.
 */
int fixed_timer_init(void)
{
   int i;

   LOCK_VARIABLE(timedrv_fixed_rate);
   LOCK_VARIABLE(bios_counter);
   LOCK_FUNCTION(fixed_timer_handler);

   bios_counter = 0x10000;

   if (_install_irq(TIMER_INT, fixed_timer_handler) != 0)
      return -1;

   DISABLE();

   /* sometimes it doesn't seem to register if we only do this once... */
   for (i=0; i<4; i++)
      set_timer_rate(LOVEBILL_TIMER_SPEED);

   ENABLE();

   return 0;
}



/* fixed_timer_exit:
 *  Shuts down the fixed-rate timer driver.
 */
void fixed_timer_exit(void)
{
   DISABLE();

   set_timer_rate(0);

   _remove_irq(TIMER_INT);

   set_timer_rate(0);

   ENABLE();
}



/* var_timer_handler:
 *  Interrupt handler for the variable-rate timer driver.
 */
static int var_timer_handler(void)
{
   long new_delay = 0x8000;
   int callback[MAX_TIMERS];
   int bios = FALSE;
   int x;

   /* reentrant interrupt? */
   if (timer_semaphore) {
      timer_delay += REENTRANT_RECALL_GAP + read_timer();
      set_timer(REENTRANT_RECALL_GAP - timer_clocking_loss/2);
      outportb(0x20, 0x20); 
      return 0;
   }

   timer_semaphore = TRUE;

   /* deal with retrace synchronisation */
   vsync_counter -= timer_delay; 

   if (vsync_counter <= 0) {
      if (_timer_use_retrace) {
	 /* wait for retrace to start */
	 do { 
	 } while (!(inportb(0x3DA)&8));

	 /* update the VGA pelpan register? */
	 if (_retrace_hpp_value >= 0) {
	    inportb(0x3DA);
	    outportb(0x3C0, 0x33);
	    outportb(0x3C0, _retrace_hpp_value);
	    _retrace_hpp_value = -1;
	 }

	 /* user callback */
	 retrace_count++;
	 if (retrace_proc)
	    retrace_proc();

	 vsync_counter = vsync_speed - VSYNC_MARGIN;
      }
      else {
	 vsync_counter += vsync_speed;
	 retrace_count++;
	 if (retrace_proc)
	    retrace_proc();
      }
   }

   if (vsync_counter < new_delay)
      new_delay = vsync_counter;

   timer_delay += read_timer() + timer_clocking_loss;
   set_timer(0);

   /* process the user callbacks */
   for (x=0; x<MAX_TIMERS; x++) { 
      callback[x] = FALSE;

      if (((_timer_queue[x].proc) || (_timer_queue[x].param_proc)) && 
	  (_timer_queue[x].speed > 0)) {
	 _timer_queue[x].counter -= timer_delay;

	 if (_timer_queue[x].counter <= 0) {
	    _timer_queue[x].counter += _timer_queue[x].speed;
	    callback[x] = TRUE;
	 }

	 if ((_timer_queue[x].counter > 0) && (_timer_queue[x].counter < new_delay))
	    new_delay = _timer_queue[x].counter;
      }
   }

   /* update bios time */
   bios_counter -= timer_delay; 

   if (bios_counter <= 0) {
      bios_counter += 0x10000;
      bios = TRUE;
   }

   if (bios_counter < new_delay)
      new_delay = bios_counter;

   /* fudge factor to prevent interrupts coming too close to each other */
   if (new_delay < 1024)
      timer_delay = 1024;
   else
      timer_delay = new_delay;

   /* start the timer up again */
   new_delay = read_timer();
   set_timer(timer_delay);
   timer_delay += new_delay;

   if (!bios) {
      outportb(0x20, 0x20);      /* ack. the interrupt */
      ENABLE();
   }

   /* finally call the user timer routines */
   for (x=0; x<MAX_TIMERS; x++) {
      if (callback[x]) {
	 if (_timer_queue[x].param_proc)
	    _timer_queue[x].param_proc(_timer_queue[x].param);
	 else
	    _timer_queue[x].proc();

	 while (((_timer_queue[x].proc) || (_timer_queue[x].param_proc)) && 
		(_timer_queue[x].counter <= 0)) {
	    _timer_queue[x].counter += _timer_queue[x].speed;
	    if (_timer_queue[x].param_proc)
	       _timer_queue[x].param_proc(_timer_queue[x].param);
	    else
	       _timer_queue[x].proc();
	 }
      }
   }

   if (!bios)
      DISABLE();

   timer_semaphore = FALSE;

   return bios;
}

END_OF_STATIC_FUNCTION(var_timer_handler);



/* var_timer_can_simulate_retrace:
 *  Checks whether it is cool to enable retrace syncing at the moment.
 */
int var_timer_can_simulate_retrace(void)
{
   return ((gfx_driver) && ((gfx_driver->id == GFX_VGA) || 
			    (gfx_driver->id == GFX_MODEX)));
}



/* timer_calibrate_retrace:
 *  Times several vertical retraces, and calibrates the retrace syncing
 *  code accordingly.
 */
static void timer_calibrate_retrace(void)
{
   int ot = _timer_use_retrace;
   int c;

   #define AVERAGE_COUNT   4

   _timer_use_retrace = FALSE;
   vsync_speed = 0;

   /* time several retraces */
   for (c=0; c<AVERAGE_COUNT; c++) {
      _enter_critical();
      _vga_vsync();
      set_timer(0);
      _vga_vsync();
      vsync_speed += read_timer();
      _exit_critical();
   }

   set_timer(timer_delay);

   vsync_speed /= AVERAGE_COUNT;

   /* sanity check to discard bogus values */
   if ((vsync_speed > BPS_TO_TIMER(40)) || (vsync_speed < BPS_TO_TIMER(110)))
      vsync_speed = BPS_TO_TIMER(70);

   vsync_counter = vsync_speed;
   _timer_use_retrace = ot;
}



/* var_timer_simulate_retrace:
 *  Turns retrace syncing mode on or off.
 */
void var_timer_simulate_retrace(int enable)
{
   if (enable) {
      timer_calibrate_retrace();
      _timer_use_retrace = TRUE;
   }
   else {
      _timer_use_retrace = FALSE;
      vsync_counter = vsync_speed = BPS_TO_TIMER(70);
   }
}



/* var_timer_init:
 *  Installs the variable-rate timer driver.
 */
int var_timer_init(void)
{
   int x, y;

   if (i_love_bill)
      return -1;

   LOCK_VARIABLE(timedrv_variable_rate);
   LOCK_VARIABLE(bios_counter);
   LOCK_VARIABLE(timer_delay);
   LOCK_VARIABLE(timer_semaphore);
   LOCK_VARIABLE(timer_clocking_loss);
   LOCK_VARIABLE(vsync_counter);
   LOCK_VARIABLE(vsync_speed);
   LOCK_FUNCTION(var_timer_handler);

   vsync_counter = vsync_speed = BPS_TO_TIMER(70);

   bios_counter = 0x10000;
   timer_delay = 0x10000;

   if (_install_irq(TIMER_INT, var_timer_handler) != 0)
      return -1;

   DISABLE();

   /* now work out how much time calls to the 8254 clock chip take 
    * on this CPU/motherboard combination. It is impossible to time 
    * a set_timer call so we approximate it by a read_timer call with 
    * 4/5ths of a read_timer (no maths and no final read wait). This 
    * gives 9/10ths of 4 read_timers. Do it three times all over to 
    * get an averaging effect.
    */
   x = read_timer();
   read_timer();
   read_timer();
   read_timer();
   y = read_timer();
   read_timer();
   read_timer();
   read_timer();
   y = read_timer();
   read_timer();
   read_timer();
   read_timer();
   y = read_timer();

   if (y >= x)
      timer_clocking_loss = y - x;
   else
      timer_clocking_loss = 0x10000 - x + y;

   timer_clocking_loss = (9*timer_clocking_loss)/30;

   /* sometimes it doesn't seem to register if we only do this once... */
   for (x=0; x<4; x++)
      set_timer(timer_delay);

   ENABLE();

   return 0;
}



/* var_timer_exit:
 *  Shuts down the variable-rate timer driver.
 */
void var_timer_exit(void)
{
   DISABLE();

   set_timer_rate(0);

   _remove_irq(TIMER_INT);

   set_timer_rate(0);

   ENABLE();
}

