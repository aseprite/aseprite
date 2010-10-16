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
 *      Asynchronous I/O helpers for Linux Allegro.
 *
 *      By Marek Habersack, mangled by George Foot.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>


static SIGIO_HOOK user_sigio_hook = NULL;

#define DEFAULT_ASYNC_IO_MODE    ASYNC_BSD

unsigned __al_linux_async_io_mode = 0;


/* al_linux_install_sigio_hook:
 *  Install a hook to be called from inside the SIGIO signal handler.  
 *  It will be invoked AFTER the standard drivers are called.  Note
 *  that it's not a very good way to handle SIGIO in your application.
 *  If you really need to use SIGIO, consider updating input events 
 *  by hand, i.e. synchronously.  Or, preferably, use the select(2) 
 *  facility for asynchronous input.
 *
 *  The previous hook is returned, NULL if none.
 */
SIGIO_HOOK al_linux_install_sigio_hook (SIGIO_HOOK hook)
{
   SIGIO_HOOK ret = user_sigio_hook;
   user_sigio_hook = hook;
   return ret;
}


/* async_io_event:
 *  A handler for the SIGIO signal.  It used to be inelegant, more 
 *  optimised for speed, but I changed that because it was causing 
 *  problems (mouse motion stopped the keyboard responding).
 */
static RETSIGTYPE async_io_event(int signo)
{
   if (__al_linux_std_drivers[STD_MOUSE]) __al_linux_std_drivers[STD_MOUSE]->update();
   if (__al_linux_std_drivers[STD_KBD]) __al_linux_std_drivers[STD_KBD]->update();
   if (user_sigio_hook) user_sigio_hook(SIGIO);
   return;
}



/* al_linux_set_async_mode:
 *  Sets the asynchronous I/O mode for the registered standard device
 *  drivers.  The async I/O is based on the BSD-compatible SIGIO signal.
 */
int al_linux_set_async_mode (unsigned type)
{
   static struct sigaction org_sigio;
   struct sigaction sa;
   
   if (type == ASYNC_DEFAULT)
      type = DEFAULT_ASYNC_IO_MODE;

   /* Turn off drivers */
   __al_linux_async_set_drivers (__al_linux_async_io_mode, 0);

   /* Shut down the previous mode */
   switch (__al_linux_async_io_mode) {
      case ASYNC_BSD:
         sigaction (SIGIO, &org_sigio, NULL);
         break;
   }

   __al_linux_async_io_mode = type;

   /* Initialise the new mode */
   switch (__al_linux_async_io_mode) {
      case ASYNC_BSD:
         sa.sa_flags   = SA_RESTART;
         sa.sa_handler = async_io_event;
         sigfillset (&sa.sa_mask);
         sigaction (SIGIO, &sa, &org_sigio);
         break;
   }

   /* Turn drivers back on again */
   __al_linux_async_set_drivers (__al_linux_async_io_mode, 1);

   return 0;
}


int al_linux_is_async_mode (void)
{
   return (__al_linux_async_io_mode != ASYNC_OFF);
}


