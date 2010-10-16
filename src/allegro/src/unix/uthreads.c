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
 *      Asynchronous event processing with pthreads.
 *
 *      By George Foot and Peter Wang.
 *
 *      Synchronization functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_HAVE_LIBPTHREAD

#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <limits.h>


#ifndef ALLEGRO_MACOSX

static void bg_man_pthreads_enable_interrupts(void);
static void bg_man_pthreads_disable_interrupts(void);

#define MAX_FUNCS 16

static bg_func funcs[MAX_FUNCS];
static int max_func; /* highest+1 used entry */

static pthread_t thread = 0;
static int thread_alive = FALSE;
static pthread_mutex_t cli_mutex;
static pthread_cond_t cli_cond;
static int cli_count;



/* block_all_signals:
 *  Blocks all signals for the calling thread.
 */
static void block_all_signals(void)
{
   sigset_t mask;
   sigfillset(&mask);
   pthread_sigmask(SIG_BLOCK, &mask, NULL);
}



/* bg_man_pthreads_threadfunc:
 *  Thread function for the background thread.
 */
static void *bg_man_pthreads_threadfunc(void *arg)
{
   struct timeval old_time, new_time;
   struct timeval delay;
   unsigned long interval;
   int n;

   block_all_signals();

   interval = 0;
   gettimeofday(&old_time, 0);

   while (thread_alive) {
      gettimeofday(&new_time, 0);
      /* add the new time difference to the remainder of the old difference */
      interval += ((new_time.tv_sec - old_time.tv_sec) * 1000000L +
		   (new_time.tv_usec - old_time.tv_usec));
      old_time = new_time;

      /* run the callbacks for each 10ms elapsed, but limit to 18ms */
      if (interval > 18000)
         interval = 18000;      
      while (interval > 10000) {
	 interval -= 10000;	  

	 pthread_mutex_lock(&cli_mutex);

	 /* wait until interrupts are enabled */
	 while (cli_count > 0)
	    pthread_cond_wait(&cli_cond, &cli_mutex);

	 /* call all the callbacks */
	 for (n = 0; n < max_func; n++) {
	    if (funcs[n])
	       funcs[n](1);
	 }

	 pthread_mutex_unlock(&cli_mutex);
      }

      /* rest a little bit before checking again */
      delay.tv_sec = 0;
      delay.tv_usec = 1000;
      select(0, NULL, NULL, NULL, &delay);
   }

   return NULL;
}



/* bg_man_pthreads_init:
 *  Spawns the background thread.
 */
static int bg_man_pthreads_init(void)
{
   int i;

   ASSERT(thread == 0);
   ASSERT(!thread_alive);

   for (i = 0; i < MAX_FUNCS; i++)
      funcs[i] = NULL;

   max_func = 0;

   cli_count = 0;
   pthread_mutex_init(&cli_mutex, NULL);
   pthread_cond_init(&cli_cond, NULL);

   thread_alive = TRUE;
   if (pthread_create(&thread, NULL, bg_man_pthreads_threadfunc, NULL)) {
      thread_alive = FALSE;
      pthread_mutex_destroy(&cli_mutex);
      pthread_cond_destroy(&cli_cond);
      thread = 0;
      return -1;
   }

   return 0;
}



/* bg_man_pthreads_exit:
 *  Kills the background thread.
 */
static void bg_man_pthreads_exit(void)
{
   ASSERT(!!thread == !!thread_alive);

   if (thread) {
      thread_alive = FALSE;
      pthread_join(thread, NULL);
      pthread_mutex_destroy(&cli_mutex);
      pthread_cond_destroy(&cli_cond);
      thread = 0;
   }
}



/* bg_man_pthreads_register_func:
 *  Registers a function to be called by the background thread.
 */
static int bg_man_pthreads_register_func(bg_func f)
{
   int i, ret = 0;

   bg_man_pthreads_disable_interrupts();

   for (i = 0; i < MAX_FUNCS && funcs[i]; i++)
      ;

   if (i == MAX_FUNCS)
      ret = -1;
   else {
      funcs[i] = f;
      if (i == max_func)
	 max_func++;
   }

   bg_man_pthreads_enable_interrupts();

   return ret;
}



/* really_unregister_func:
 *  Unregisters a function registered with bg_man_pthreads_register_func.
 */
static int really_unregister_func(bg_func f)
{
   int i;

   for (i = 0; i < max_func && funcs[i] != f; i++)
      ;

   if (i == max_func)
      return -1;
   else {
      funcs[i] = NULL;
      if (i+1 == max_func)
	 do {
	    max_func--;
	 } while ((max_func > 0) && !funcs[max_func-1]);
      return 0;
   }
}



/* bg_man_pthreads_unregister_func:
 *  Wrapper for really_unregister_func to avoid locking problems.
 */
static int bg_man_pthreads_unregister_func(bg_func f)
{
   /* Normally we just bg_man_pthread_disable_interrupts(), remove `f',
    * then bg_man_pthread_enable_interrupts().
    *
    * However, the X system driver's input handler is a bg_man callback.
    * When it receives a close event, it calls exit(), which calls
    * allegro_exit().  Eventually various subsystems will try to
    * unregister their callbacks, i.e. call this function.  But we're in
    * the middle of input handler call, and `cli_mutex' is still locked.
    *
    * So we... special case!  If the calling thread is the bg_man thread
    * we simply bypass the locking, as there's no synchronisation
    * problem to avoid anyway.
    */

   int ret;

   if (pthread_equal(pthread_self(), thread))
      /* Being called from a bg_man registered callback.  */
      ret = really_unregister_func(f);
   else {
      /* Normal case.  */
      bg_man_pthreads_disable_interrupts();
      ret = really_unregister_func(f);
      bg_man_pthreads_enable_interrupts();
   }

   return ret;
}



/* bg_man_pthreads_enable_interrupts:
 *  Enables interrupts for the background thread.
 */
static void bg_man_pthreads_enable_interrupts(void)
{
   pthread_mutex_lock(&cli_mutex);
   if (--cli_count == 0)
      pthread_cond_signal(&cli_cond);
   pthread_mutex_unlock(&cli_mutex);
}



/* bg_man_pthreads_disable_interrupts:
 *  Disables interrupts for the background thread.
 */
static void bg_man_pthreads_disable_interrupts(void)
{
   pthread_mutex_lock(&cli_mutex);
   cli_count++;
   pthread_mutex_unlock(&cli_mutex);
}



/* bg_man_pthreads_interrupts_disabled:
 *  Returns whether interrupts are disabled for the background thread.
 */
static int bg_man_pthreads_interrupts_disabled(void)
{
   return cli_count;
}



/* the background manager object */
struct bg_manager _bg_man_pthreads = {
   1,
   bg_man_pthreads_init,
   bg_man_pthreads_exit,
   bg_man_pthreads_register_func,
   bg_man_pthreads_unregister_func,
   bg_man_pthreads_enable_interrupts,
   bg_man_pthreads_disable_interrupts,
   bg_man_pthreads_interrupts_disabled
};

#endif /* !ALLEGRO_MACOSX */



/* custom mutex that supports nested locking */
struct my_mutex {
   int lock_count;                /* level of nested locking     */
   pthread_t owner;               /* thread which owns the mutex */
   pthread_mutex_t actual_mutex;  /* underlying mutex object     */
};



/* _unix_create_mutex:
 *  Creates a mutex and returns a pointer to it.
 */
void *_unix_create_mutex(void)
{
   struct my_mutex *mx;

   mx = _AL_MALLOC(sizeof(struct my_mutex));
   if (!mx) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   mx->lock_count = 0;
   mx->owner = (pthread_t) 0;

   pthread_mutex_init(&mx->actual_mutex, NULL);

   return (void *)mx;
}



/* _unix_destroy_mutex:
 *  Destroys a mutex.
 */
void _unix_destroy_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   pthread_mutex_destroy(&mx->actual_mutex);

   _AL_FREE(mx);
}



/* _unix_lock_mutex:
 *  Locks a mutex.
 */
void _unix_lock_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   if (mx->owner != pthread_self()) {
      pthread_mutex_lock(&mx->actual_mutex);
      mx->owner = pthread_self();      
   }

   mx->lock_count++;
}



/* _unix_unlock_mutex:
 *  Unlocks a mutex.
 */
void _unix_unlock_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   mx->lock_count--;

   if (mx->lock_count == 0) {
      mx->owner = (pthread_t) 0;
      pthread_mutex_unlock(&mx->actual_mutex);
   }
}

#endif	/* ALLEGRO_HAVE_LIBPTHREAD */

