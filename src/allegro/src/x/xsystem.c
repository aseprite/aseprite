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
 *      Main system driver for the X-Windows library.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>



void (*_xwin_keyboard_interrupt)(int pressed, int code) = 0;
void (*_xwin_keyboard_focused)(int focused, int state) = 0;
void (*_xwin_mouse_interrupt)(int x, int y, int z, int w, int buttons) = 0;


static int _xwin_sysdrv_init(void);
static void _xwin_sysdrv_exit(void);
static void _xwin_sysdrv_set_window_title(AL_CONST char *name);
static int _xwin_sysdrv_set_close_button_callback(void (*proc)(void));
static void _xwin_sysdrv_message(AL_CONST char *msg);
static int _xwin_sysdrv_display_switch_mode(int mode);
static int _xwin_sysdrv_desktop_color_depth(void);
static int _xwin_sysdrv_get_desktop_resolution(int *width, int *height);
static void _xwin_sysdrv_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
static _DRIVER_INFO *_xwin_sysdrv_gfx_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_digi_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_midi_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_keyboard_drivers(void);
static _DRIVER_INFO *_xwin_sysdrv_mouse_drivers(void);
#ifdef ALLEGRO_HAVE_LINUX_JOYSTICK_H
static _DRIVER_INFO *_xwin_sysdrv_joystick_drivers(void);
#endif
static _DRIVER_INFO *_xwin_sysdrv_timer_drivers(void);


/* the main system driver for running under X-Windows */
SYSTEM_DRIVER system_xwin =
{
   SYSTEM_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows",
   _xwin_sysdrv_init,
   _xwin_sysdrv_exit,
   _unix_get_executable_name,
   _unix_find_resource,
   _xwin_sysdrv_set_window_title,
   _xwin_sysdrv_set_close_button_callback,
   _xwin_sysdrv_message,
   NULL, /* assert */
   NULL, /* save_console_state */
   NULL, /* restore_console_state */
   NULL, /* create_bitmap */
   NULL, /* created_bitmap */
   NULL, /* create_sub_bitmap */
   NULL, /* created_sub_bitmap */
   NULL, /* destroy_bitmap */
   NULL, /* read_hardware_palette */
   NULL, /* set_palette_range */
   NULL, /* get_vtable */
   _xwin_sysdrv_display_switch_mode,
   NULL, /* display_switch_lock */
   _xwin_sysdrv_desktop_color_depth,
   _xwin_sysdrv_get_desktop_resolution,
   _xwin_sysdrv_get_gfx_safe_mode,
   _unix_yield_timeslice,
#ifdef ALLEGRO_HAVE_LIBPTHREAD
   _unix_create_mutex,
   _unix_destroy_mutex,
   _unix_lock_mutex,
   _unix_unlock_mutex,
#else
   NULL, /* create_mutex */
   NULL, /* destroy_mutex */
   NULL, /* lock_mutex */
   NULL, /* unlock_mutex */
#endif
   _xwin_sysdrv_gfx_drivers,
   _xwin_sysdrv_digi_drivers,
   _xwin_sysdrv_midi_drivers,
   _xwin_sysdrv_keyboard_drivers,
   _xwin_sysdrv_mouse_drivers,
#ifdef ALLEGRO_HAVE_LINUX_JOYSTICK_H
   _xwin_sysdrv_joystick_drivers,
#else
   NULL, /* joystick_driver_list */
#endif
   _xwin_sysdrv_timer_drivers
};


static RETSIGTYPE (*old_sig_abrt)(int num);
static RETSIGTYPE (*old_sig_fpe)(int num);
static RETSIGTYPE (*old_sig_ill)(int num);
static RETSIGTYPE (*old_sig_segv)(int num);
static RETSIGTYPE (*old_sig_term)(int num);
static RETSIGTYPE (*old_sig_int)(int num);
#ifdef SIGQUIT
static RETSIGTYPE (*old_sig_quit)(int num);
#endif

/* _xwin_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE _xwin_signal_handler(int num)
{
   if (_unix_bg_man->interrupts_disabled() || _xwin.lock_count) {
      /* Can not shutdown X-Windows, restore old signal handlers and slam the door.  */
      signal(SIGABRT, old_sig_abrt);
      signal(SIGFPE,  old_sig_fpe);
      signal(SIGILL,  old_sig_ill);
      signal(SIGSEGV, old_sig_segv);
      signal(SIGTERM, old_sig_term);
      signal(SIGINT,  old_sig_int);
#ifdef SIGQUIT
      signal(SIGQUIT, old_sig_quit);
#endif
      raise(num);
      abort();
   }
   else {
      allegro_exit();
      fprintf(stderr, "Shutting down Allegro due to signal #%d\n", num);
      raise(num);
   }
}



/* _xwin_bg_handler:
 *  Really used for synchronous stuff.
 */
static void _xwin_bg_handler(int threaded)
{
   _xwin_handle_input();
}


/* _xwin_sysdrv_init:
 *  Top level system driver wakeup call.
 */
static int _xwin_sysdrv_init(void)
{
   char tmp[256];

   _unix_read_os_type();

   /* install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, _xwin_signal_handler);
   old_sig_fpe  = signal(SIGFPE,  _xwin_signal_handler);
   old_sig_ill  = signal(SIGILL,  _xwin_signal_handler);
   old_sig_segv = signal(SIGSEGV, _xwin_signal_handler);
   old_sig_term = signal(SIGTERM, _xwin_signal_handler);
   old_sig_int  = signal(SIGINT,  _xwin_signal_handler);

#ifdef SIGQUIT
   old_sig_quit = signal(SIGQUIT, _xwin_signal_handler);
#endif

   /* Initialise dynamic driver lists and load modules */
   _unix_driver_lists_init();
   if (_unix_gfx_driver_list)
      _driver_list_append_list(&_unix_gfx_driver_list, _xwin_gfx_driver_list);
   _unix_load_modules(SYSTEM_XWINDOWS);

#ifdef ALLEGRO_HAVE_LIBPTHREAD
   _unix_bg_man = &_bg_man_pthreads;
#else
   _unix_bg_man = &_bg_man_sigalrm;
#endif

   /* Initialise bg_man */
   if (_unix_bg_man->init()) {
      _xwin_sysdrv_exit();
      return -1;
   }

#ifdef ALLEGRO_MULTITHREADED
   if (_unix_bg_man->multi_threaded) {
      _xwin.mutex = _unix_create_mutex();
   }
#endif

   get_executable_name(tmp, sizeof(tmp));
   set_window_title(get_filename(tmp));

   if (get_config_int("system", "XInitThreads", 1))
      XInitThreads();

   /* Open the display, create a window, and background-process 
    * events for it all. */
   if (_xwin_open_display(0) || _xwin_create_window()
       || _unix_bg_man->register_func(_xwin_bg_handler))
   {
      _xwin_sysdrv_exit();
      return -1;
   }

   set_display_switch_mode(SWITCH_BACKGROUND);

   return 0;
}



/* _xwin_sysdrv_exit:
 *  The end of the world...
 */
static void _xwin_sysdrv_exit(void)
{
   /* This stops the X event handler running in the background, which
    * seems a nice thing to do before closing the connection to the X
    * display... (remove this and you get SIGABRTs during XCloseDisplay).
    */
   _unix_bg_man->unregister_func(_xwin_bg_handler);

   _xwin_close_display();
   _unix_bg_man->exit();

   _unix_unload_modules();
   _unix_driver_lists_shutdown();

   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);

#ifdef SIGQUIT
   signal(SIGQUIT, old_sig_quit);
#endif

#ifdef ALLEGRO_MULTITHREADED
   if (_xwin.mutex) {
      _unix_destroy_mutex(_xwin.mutex);
      _xwin.mutex = NULL;
   }
#endif
}



/* _xwin_sysdrv_set_window_title:
 *  Sets window title.
 */
static void _xwin_sysdrv_set_window_title(AL_CONST char *name)
{
   char title[128];

   do_uconvert(name, U_CURRENT, title, U_ASCII, sizeof(title));

   _xwin_set_window_title(title);
}



/* _xwin_sysdrv_set_close_button_callback:
 *  Sets close button callback function.
 */
static int _xwin_sysdrv_set_close_button_callback(void (*proc)(void))
{
   _xwin.close_button_callback = proc;
   
   return 0;
}



/* _xwin_sysdrv_message:
 *  Displays a message. Uses xmessage if possible, and stdout if not.
 */
static void _xwin_sysdrv_message(AL_CONST char *msg)
{
   char buf[ALLEGRO_MESSAGE_SIZE+1];
   char *msg2;
   size_t len;
   pid_t pid;
   int status;

   /* convert message to ASCII */
   msg2 = uconvert(msg, U_CURRENT, buf, U_ASCII, ALLEGRO_MESSAGE_SIZE);

   /* xmessage interprets some strings beginning with '-' as command-line
    * options. To avoid this we make sure all strings we pass to it have
    * newlines on the end. This is also useful for the fputs() case.
    */
   len = strlen(msg2);
   ASSERT(len < ALLEGRO_MESSAGE_SIZE);
   if ((len == 0) || (msg2[len-1] != '\n'))
      strcat(msg2, "\n");

   /* fork a child */
   pid = fork();
   switch (pid) {

      case -1: /* fork error */
	 fputs(msg2, stdout);
	 break;

      case 0: /* child process */
	 execlp("xmessage", "xmessage", "-buttons", "OK:101", "-default", "OK", "-center", msg2, (char *)NULL);

	 /* if execution reaches here, it means execlp failed */
	 _exit(EXIT_FAILURE);
	 break;

      default: /* parent process */
	 waitpid(pid, &status, 0);
	 if ((!WIFEXITED(status))
	     || (WEXITSTATUS(status) != 101)) /* ok button */
	 {
	    fputs(msg2, stdout);
	 }

	 break;
   }
}



/* _xwin_sysdrv_gfx_drivers:
 *  Get the list of graphics drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_gfx_drivers(void)
{
   return _unix_gfx_driver_list;
}



/* _xwin_sysdrv_digi_drivers:
 *  Get the list of digital sound drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_digi_drivers(void)
{
   return _unix_digi_driver_list;
}



/* _xwin_sysdrv_midi_drivers:
 *  Get the list of MIDI drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_midi_drivers(void)
{
   return _unix_midi_driver_list;
}



/* _xwin_sysdrv_keyboard_drivers:
 *  Get the list of keyboard drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_keyboard_drivers(void)
{
   return _xwin_keyboard_driver_list;
}



/* _xwin_sysdrv_mouse_drivers:
 *  Get the list of mouse drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_mouse_drivers(void)
{
   return _xwin_mouse_driver_list;
}



#ifdef ALLEGRO_HAVE_LINUX_JOYSTICK_H
/* _xwin_sysdrv_joystick_drivers:
 *  Get the list of joystick drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_joystick_drivers(void)
{
   return _linux_joystick_driver_list;
}
#endif



/* _xwin_sysdrv_timer_drivers:
 *  Get the list of timer drivers.
 */
static _DRIVER_INFO *_xwin_sysdrv_timer_drivers(void)
{
   return _xwin_timer_driver_list;
}



/* _xwin_sysdrv_display_switch_mode:
 *  Tries to set the display switching mode (this is mostly just to
 *  return sensible values to the caller: most of the modes don't
 *  properly apply to X).
 */
static int _xwin_sysdrv_display_switch_mode(int mode)
{
   if (_xwin.in_dga_mode) {
      if (mode != SWITCH_NONE)
	 return -1;
   }

   if (mode != SWITCH_BACKGROUND)
      return -1;

   return 0;
}



/* _xwin_sysdrv_desktop_color_depth:
 *  Returns the current desktop color depth.
 */
static int _xwin_sysdrv_desktop_color_depth(void)
{
   if (_xwin.window_depth <= 8)
      return 8;
   else if (_xwin.window_depth <= 15)
      return 15;
   else if (_xwin.window_depth == 16)
      return 16;
   else
      return 32;
}



/* _xwin_sysdrv_get_desktop_resolution:
 *  Returns the current desktop resolution.
 */
static int _xwin_sysdrv_get_desktop_resolution(int *width, int *height)
{
   XLOCK();
   *width = DisplayWidth(_xwin.display, _xwin.screen);
   *height = DisplayHeight(_xwin.display, _xwin.screen);
   XUNLOCK();
   return 0;
}



/* _xwin_sysdrv_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void _xwin_sysdrv_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_XWINDOWS;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}

