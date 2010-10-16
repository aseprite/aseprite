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
 *      Timer driver for the PSP.
 *      TODO: Thread synchronization.
 * 
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include <pspthreadman.h>
#include <psprtc.h>


#define TIMER_TO_USEC(t)      ((SceUInt)((float)(t) / TIMERS_PER_SECOND * 1000000))
#define PSPTIMER_TO_TIMER(t)  ((int)((t) * (TIMERS_PER_SECOND / (float)psp_tick_resolution)))


static int psp_timer_thread();
static int psp_timer_init(void);
static void psp_timer_exit(void);


static uint32_t psp_tick_resolution;
static int psp_timer_on = FALSE;
static SceUID timer_thread_UID;


TIMER_DRIVER timer_psp =
{
   TIMER_PSP,
   empty_string,
   empty_string,
   "PSP timer",
   psp_timer_init,
   psp_timer_exit,
   NULL, NULL,   /* install_int, remove_int */
   NULL, NULL,   /* install_param_int, remove_param_int */
   NULL, NULL,   /* can_simulate_retrace, simulate_retrace */
   NULL          /* rest */
};



/* psp_timer_thread:
 *  This PSP thread measures the elapsed time.
 */
static int psp_timer_thread()
{
   uint64_t old_tick, new_tick;
   int interval;
   long delay;

   sceRtcGetCurrentTick(&old_tick);

   while (psp_timer_on) {
      /* Calculate actual time elapsed. */
      sceRtcGetCurrentTick(&new_tick);
      interval = PSPTIMER_TO_TIMER(new_tick - old_tick);
      old_tick = new_tick;

      /* Handle a tick and rest. */
      delay = _handle_timer_tick(interval);
      sceKernelDelayThreadCB(TIMER_TO_USEC(delay));
   }
   
   sceKernelExitThread(0);

   return 0;
}



/* psp_timer_init:
 *  Installs the PSP timer thread.
 */
static int psp_timer_init(void)
{
   /* Get the PSP ticks per second */
   psp_tick_resolution = sceRtcGetTickResolution();
   
   psp_timer_on = TRUE;

   timer_thread_UID = sceKernelCreateThread("psp_timer_thread",(void *)&psp_timer_thread,
                                             0x18, 0x10000, 0, NULL);
   
   if (timer_thread_UID < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create timer thread"));
      psp_timer_exit();
      return -1;
   }
   
   if (sceKernelStartThread(timer_thread_UID, 0, NULL) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot start timer thread"));
      psp_timer_exit();
      return -1;
   }

   return 0;
}



/* psp_timer_exit:
 *  Shuts down the PSP timer thread.
 */
static void psp_timer_exit(void)
{
   psp_timer_on = FALSE;
   sceKernelDeleteThread(timer_thread_UID);
   
   return;
}
