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
 *      System driver for the QNX realtime platform.
 *
 *      By Angelo Mottola.
 *
 *      Signals handling derived from Unix version by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
   #error something is wrong with the makefile
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <signal.h>
   #include <sys/time.h>
   #include <unistd.h>
   #include <sched.h>
#endif


static int qnx_sys_init(void);
static void qnx_sys_exit(void);
static void qnx_sys_message(AL_CONST char *);
static void qnx_sys_set_window_title(AL_CONST char *);
static int qnx_sys_set_close_button_callback(void (*proc)(void));
static int qnx_sys_set_display_switch_mode(int mode);
static void qnx_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
static void qnx_sys_yield_timeslice(void);
static int qnx_sys_desktop_color_depth(void);
static int qnx_sys_get_desktop_resolution(int *width, int *height);


SYSTEM_DRIVER system_qnx =
{
   SYSTEM_QNX,
   empty_string,
   empty_string,
   "QNX Realtime Platform",
   qnx_sys_init,
   qnx_sys_exit,
   _unix_get_executable_name,
   _unix_find_resource,
   qnx_sys_set_window_title,
   qnx_sys_set_close_button_callback,
   qnx_sys_message,
   NULL,  /* AL_METHOD(void, assert, (AL_CONST char *msg)); */
   NULL,  /* AL_METHOD(void, save_console_state, (void)); */
   NULL,  /* AL_METHOD(void, restore_console_state, (void)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,  /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   NULL,  /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent)); */
   NULL,  /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,  /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,  /* AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,  /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   qnx_sys_set_display_switch_mode,
   NULL,  /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
   qnx_sys_desktop_color_depth,
   qnx_sys_get_desktop_resolution,
   qnx_sys_get_gfx_safe_mode,
   qnx_sys_yield_timeslice,
   _unix_create_mutex,
   _unix_destroy_mutex,
   _unix_lock_mutex,
   _unix_unlock_mutex,
   NULL,  /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};


/* system globals */
PtWidget_t *ph_window = NULL;

pthread_mutex_t qnx_event_mutex;
pthread_mutex_t qnx_gfx_mutex;

#define EVENT_SIZE  (sizeof(PhEvent_t) + 1000)

static PhEvent_t *ph_event = NULL;
static void (*window_close_hook)(void) = NULL;
static int switch_mode = SWITCH_BACKGROUND;

#define WINDOW_TITLE_SIZE  256
static char window_title[WINDOW_TITLE_SIZE];


typedef void RETSIGTYPE;

static RETSIGTYPE (*old_sig_abrt)(int num);
static RETSIGTYPE (*old_sig_fpe)(int num);
static RETSIGTYPE (*old_sig_ill)(int num);
static RETSIGTYPE (*old_sig_segv)(int num);
static RETSIGTYPE (*old_sig_term)(int num);
static RETSIGTYPE (*old_sig_int)(int num);
#ifdef SIGQUIT
static RETSIGTYPE (*old_sig_quit)(int num);
#endif



/* qnx_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE qnx_signal_handler(int num)
{
   if (_unix_bg_man->interrupts_disabled()) {
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



/* qnx_event_handler:
 *  Thread to handle pending system events.
 */
static void qnx_event_handler(int threaded)
{
   static PhRegion_t region;
   static PhWindowEvent_t *window_event;
   static PhKeyEvent_t *key_event;
   static PhPointerEvent_t *mouse_event;
   static PhCursorInfo_t cursor_info;
   static int old_x = 0, old_y = 0, ig, dx, dy, buttons = 0, res, i;
   static short mx, my;
      
   pthread_mutex_lock(&qnx_event_mutex);
      
   /* special mouse handling for the fullscreen mode */
   if (_mouse_installed && (ph_gfx_mode == PH_GFX_DIRECT)) {
      PtGetAbsPosition(ph_window, &mx, &my);
      ig = PhInputGroup(NULL);
      PhQueryCursor(ig, &cursor_info);
      
      dx = cursor_info.pos.x - (mx + (SCREEN_W / 2));
      dy = cursor_info.pos.y - (my + (SCREEN_H / 2));

      if (qnx_mouse_warped) {
         dx = dy = 0;
         qnx_mouse_warped = FALSE;
      }

      buttons = (cursor_info.button_state & Ph_BUTTON_SELECT ? 0x1 : 0) |
                (cursor_info.button_state & Ph_BUTTON_MENU   ? 0x2 : 0) |
                (cursor_info.button_state & Ph_BUTTON_ADJUST ? 0x4 : 0);

      qnx_mouse_handler(dx, dy, 0, buttons);
      
      if (dx || dy)
         PhMoveCursorAbs(ig, mx + (SCREEN_W / 2), my + (SCREEN_H / 2));
   }

   /* check if there is a pending event */
   if (PhEventPeek(ph_event, EVENT_SIZE) != Ph_EVENT_MSG) {
      pthread_mutex_unlock(&qnx_event_mutex);
      return;
   }

   switch(ph_event->type) {
      
      /* mouse related events */
      case Ph_EV_PTR_MOTION_BUTTON:
      case Ph_EV_PTR_MOTION_NOBUTTON:
      case Ph_EV_BUT_PRESS:
      case Ph_EV_BUT_RELEASE:
         /* only if we are in windowed mode */
         if (_mouse_installed && (ph_gfx_mode == PH_GFX_WINDOW)) {
            mouse_event = (PhPointerEvent_t *)PhGetData(ph_event);
            
            if (qnx_mouse_warped) {
               dx = 0;
               dy = 0;
               qnx_mouse_warped = FALSE;
            }
            else {
               dx = mouse_event->pos.x - old_x;
               dy = mouse_event->pos.y - old_y;
            }
            
            buttons = (mouse_event->button_state & Ph_BUTTON_SELECT ? 0x1 : 0) |
                      (mouse_event->button_state & Ph_BUTTON_MENU   ? 0x2 : 0) |
                      (mouse_event->button_state & Ph_BUTTON_ADJUST ? 0x4 : 0);
                      
            qnx_mouse_handler(dx, dy, 0, buttons);
            
            old_x = mouse_event->pos.x;
            old_y = mouse_event->pos.y;
         }
         break;
         
      /* key press/release event */
      case Ph_EV_KEY:
         if (_keyboard_installed) {
            key_event = (PhKeyEvent_t *)PhGetData(ph_event);
            
            /* here we assume key_scan member to be always valid */
            if (key_event->key_flags & Pk_KF_Key_Down)
               qnx_keyboard_handler(TRUE, key_event->key_scan);
            else
               qnx_keyboard_handler(FALSE, key_event->key_scan);
         }
         break;

      /* expose event */
      case Ph_EV_EXPOSE:
         if (ph_gfx_mode == PH_GFX_WINDOW) {
            ph_update_window(NULL);
            PgFlush();
         }
         break;
      
      /* mouse enters/leaves window boundaries */
      case Ph_EV_BOUNDARY:
         /* only if we are in windowed mode */
         if (_mouse_installed && (ph_gfx_mode == PH_GFX_WINDOW)) {
            switch (ph_event->subtype) {
            
               case Ph_EV_PTR_ENTER:
                  PhQueryCursor(PhInputGroup(NULL), &cursor_info);
                  PtGetAbsPosition(ph_window, &mx, &my);
                  old_x = cursor_info.pos.x;
                  old_y = cursor_info.pos.y;
                  _mouse_x = old_x - mx;
                  _mouse_y = old_y - my;
                  _handle_mouse_input();
                  _mouse_on = TRUE;
                  break;
                  
               case Ph_EV_PTR_LEAVE:
                  _mouse_on = FALSE;
                  _handle_mouse_input();
                  break;
            }
         }
         break;
         
      case Ph_EV_WM:
         if ((ph_gfx_mode != PH_GFX_NONE) && (ph_event->subtype == Ph_EV_WM_EVENT)) {
            window_event = (PhWindowEvent_t *)PhGetData(ph_event);
            switch (window_event->event_f) {
            
               case Ph_WM_CLOSE:
                  if (window_close_hook)
                     window_close_hook();
                  break;
                  
               case Ph_WM_FOCUS:
                  pthread_mutex_lock(&qnx_gfx_mutex);
                  
                  if (window_event->event_state == Ph_WM_EVSTATE_FOCUS) {
                     qnx_keyboard_focused(TRUE, 0);
                     
                     PgSetPalette(ph_palette, 0, 0, 256, Pg_PALSET_HARDLOCKED, 0);
                     
                     _switch_in();
                     
                     qnx_mouse_warped = TRUE;
                  }
                  else if (window_event->event_state == Ph_WM_EVSTATE_FOCUSLOST) {
                     qnx_keyboard_focused(FALSE, 0);
                      
                     _switch_out();

                     PgSetPalette(ph_palette, 0, 0, -1, 0, 0);
                  }

                  pthread_mutex_unlock(&qnx_gfx_mutex);
                  
                  if (ph_gfx_mode == PH_GFX_WINDOW) {
                     ph_update_window(NULL);
                     PgFlush();
                  }
                  break;
            } /* end of switch (window_event->event_f) */
         }
         break;
            
      case Ph_EV_INFO:
         /*
          *  TODO: validate offscreen contexts
          */
         break;
   }

   pthread_mutex_unlock(&qnx_event_mutex);
}



/* qnx_sys_init:
 *  Initializes the QNX system driver.
 */
static int qnx_sys_init(void)
{
   PhRegion_t region;
   PtArg_t arg[6];
   PhDim_t dim;
   struct sched_param sparam;
   int spolicy;
   char tmp[WINDOW_TITLE_SIZE];

   /* install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, qnx_signal_handler);
   old_sig_fpe  = signal(SIGFPE,  qnx_signal_handler);
   old_sig_ill  = signal(SIGILL,  qnx_signal_handler);
   old_sig_segv = signal(SIGSEGV, qnx_signal_handler);
   old_sig_term = signal(SIGTERM, qnx_signal_handler);
   old_sig_int  = signal(SIGINT,  qnx_signal_handler);

#ifdef SIGQUIT
   old_sig_quit = signal(SIGQUIT, qnx_signal_handler);
#endif

   dim.w = 1;
   dim.h = 1;
   _unix_get_executable_name(tmp, WINDOW_TITLE_SIZE);
   do_uconvert(tmp, U_CURRENT, window_title, U_UTF8, WINDOW_TITLE_SIZE);
   PtSetArg(&arg[0], Pt_ARG_DIM, &dim, 0);
   PtSetArg(&arg[1], Pt_ARG_WINDOW_TITLE, window_title, 0);
   PtSetArg(&arg[2], Pt_ARG_WINDOW_MANAGED_FLAGS, Pt_FALSE, Ph_WM_CLOSE);
   PtSetArg(&arg[3], Pt_ARG_WINDOW_NOTIFY_FLAGS, Pt_TRUE, Ph_WM_CLOSE | Ph_WM_FOCUS);
   PtSetArg(&arg[4], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Ph_WM_RENDER_MAX | Ph_WM_RENDER_RESIZE);

   ph_event = (PhEvent_t *)_AL_MALLOC(EVENT_SIZE);
   
   if (!(ph_window = PtAppInit(NULL, NULL, NULL, 5, arg))) {
      qnx_sys_exit();
      return -1;
   }

   PgSetRegion(PtWidgetRid(ph_window));
   PgSetClipping(0, NULL);
   PgSetDrawBufferSize(0xffff);
   PtRealizeWidget(ph_window);

   region.cursor_type = Ph_CURSOR_NONE;
   region.events_sense = Ph_EV_WM | Ph_EV_KEY | Ph_EV_PTR_ALL | Ph_EV_EXPOSE | Ph_EV_BOUNDARY | Ph_EV_INFO;
   region.rid = PtWidgetRid(ph_window);
   PhRegionChange(Ph_REGION_CURSOR | Ph_REGION_EV_SENSE, 0, &region, NULL, NULL);

   ph_gfx_mode = PH_GFX_NONE;

   pthread_mutex_init(&qnx_event_mutex, NULL);
   pthread_mutex_init(&qnx_gfx_mutex, NULL);

   _unix_bg_man = &_bg_man_pthreads;
   if (_unix_bg_man->init()) {
      qnx_sys_exit();
      return -1;
   }

   _unix_bg_man->register_func(qnx_event_handler);

   /* Note on the thread scheduling policy: (hot topic under QNX !)
    *  - the primary thread has the lowest priority: -2
    *  - the _unix_bg_man thread has an intermediate priority: 0
    *  - the timer thread has the highest priority: +2
    */ 
   
   if (pthread_getschedparam(pthread_self(), &spolicy, &sparam) == EOK) {
      sparam.sched_priority -= 2;
      pthread_setschedparam(pthread_self(), spolicy, &sparam);
   }
   
   _unix_read_os_type();

   set_display_switch_mode(SWITCH_BACKGROUND);

   return 0;
}



/* qnx_sys_exit:
 *  Shuts down the QNX system driver.
 */
static void qnx_sys_exit(void)
{
   void *status;

   _unix_bg_man->exit();
 
   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);

#ifdef SIGQUIT
   signal(SIGQUIT, old_sig_quit);
#endif

   pthread_mutex_destroy(&qnx_event_mutex);
   pthread_mutex_destroy(&qnx_gfx_mutex);

   PtUnrealizeWidget(ph_window);
   
   if (ph_event) {
      _AL_FREE(ph_event);
      ph_event = NULL;
   }
}



/* qnx_sys_set_window_title:
 *  Sets the Photon window title.
 */
static void qnx_sys_set_window_title(AL_CONST char *name)
{
   PtArg_t arg;

   do_uconvert(name, U_CURRENT, window_title, U_UTF8, WINDOW_TITLE_SIZE);
   PtSetArg(&arg, Pt_ARG_WINDOW_TITLE, window_title, 0);
   PtSetResources(ph_window, 1, &arg);
}



/* qnx_sys_set_close_button_callback:
 *  Sets the close button callback function.
 */
static int qnx_sys_set_close_button_callback(void (*proc)(void))
{
   window_close_hook = proc;

   PtSetResource(ph_window, Pt_ARG_WINDOW_RENDER_FLAGS, 
                 (proc ? Pt_TRUE : Pt_FALSE), Ph_WM_RENDER_CLOSE);
   return 0;
}



/* qnx_sys_message:
 *  Prints out a message using a system message box.
 */
static void qnx_sys_message(AL_CONST char *msg)
{
   const char *button[] = { "&Ok" };
   char *tmp=_AL_MALLOC(ALLEGRO_MESSAGE_SIZE);

   fputs(uconvert_toascii(msg, tmp), stderr);
   
   qnx_keyboard_focused(FALSE, 0);
   pthread_mutex_lock(&qnx_event_mutex);
   DISABLE();
   
   PtAlert(ph_window, NULL, window_title, NULL,
   	   uconvert(msg, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE),
   	   NULL, 1, button, NULL, 1, 1, Pt_MODAL);
   	   
   ENABLE();
   pthread_mutex_unlock(&qnx_event_mutex);
   qnx_keyboard_focused(TRUE, 0);

   _AL_FREE(tmp);
}



/* qnx_sys_set_display_switch_mode:
 *  Sets current display switching behaviour.
 */
static int qnx_sys_set_display_switch_mode (int mode)
{
   if (mode != SWITCH_BACKGROUND)
      return -1;   
   switch_mode = mode;
   return 0;
}



/* qnx_sys_desktop_color_depth:
 *  Returns the current desktop color depth.
 */
static int qnx_sys_desktop_color_depth(void)
{
   PgDisplaySettings_t settings;
   PgVideoModeInfo_t mode_info;
   
   PgGetVideoMode(&settings);
   PgGetVideoModeInfo(settings.mode, &mode_info);
   
   return (mode_info.bits_per_pixel);
}



/* qnx_sys_get_desktop_resolution:
 *  Returns the current desktop resolution.
 */
static int qnx_sys_get_desktop_resolution(int *width, int *height)
{
   PgDisplaySettings_t settings;
   PgVideoModeInfo_t mode_info;
   
   PgGetVideoMode(&settings);
   PgGetVideoModeInfo(settings.mode, &mode_info);

   *width = mode_info.width;
   *height = mode_info.height;
   return 0;
}



/* qnx_sys_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void qnx_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_PHOTON_WIN;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}



/* qnx_sys_yield_timeslice:
 *  Gives some CPU time to other apps to play nice with multitasking.
 */
static void qnx_sys_yield_timeslice(void)
{
   usleep(0);
}



/* qnx_get_window:
 *  Returns a handle to the window.
 */
PtWidget_t *qnx_get_window(void)
{
   return ph_window;
}
